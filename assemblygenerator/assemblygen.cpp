#include "assemblygen.h"
#include "llvm-c/Core.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <string>

// Physical registers: ebx=0, ecx=1, edx=2
// eax is reserved as scratch / return value
// reg_map value of -1 means spilled to memory

static const char* regNames[] = { "%ebx", "%ecx", "%edx" };
static const int REG_EBX = 0;
static const int REG_ECX = 1;
static const int REG_EDX = 2;
static const int SPILLED = -1;

// Register allocation

static std::map<LLVMValueRef, int> reg_map;        // instruction -> physical reg or -1
static std::map<LLVMValueRef, int> inst_index;      // instruction -> index in BB
static std::map<LLVMValueRef, std::pair<int,int>> live_range; // instruction -> (start, end)

static void compute_liveness(LLVMBasicBlockRef bb) {
    inst_index.clear();
    live_range.clear();

    // Build inst_index, skipping alloca
    int idx = 0;
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb);
         inst; inst = LLVMGetNextInstruction(inst)) {
        if (LLVMGetInstructionOpcode(inst) == LLVMAlloca) continue;
        inst_index[inst] = idx++;
    }

    // For each instruction that produces a value, compute live range
    for (auto& kv : inst_index) {
        LLVMValueRef inst = kv.first;
        int defIdx = kv.second;

        // Check if this instruction produces a value (has uses or is arithmetic/load/call/icmp)
        LLVMOpcode op = LLVMGetInstructionOpcode(inst);
        // Instructions that don't produce values: store, br, ret, unreachable, (void call)
        bool producesValue = true;
        if (op == LLVMStore || op == LLVMBr || op == LLVMRet || op == LLVMUnreachable) {
            producesValue = false;
        }
        if (op == LLVMCall) {
            LLVMTypeRef retTy = LLVMTypeOf(inst);
            if (LLVMGetTypeKind(retTy) == LLVMVoidTypeKind) {
                producesValue = false;
            }
        }

        if (!producesValue) continue;

        int lastUse = defIdx; // at minimum, live at definition

        // Scan all instructions in this BB for uses of inst
        for (auto& kv2 : inst_index) {
            LLVMValueRef other = kv2.first;
            int otherIdx = kv2.second;
            int numOps = LLVMGetNumOperands(other);
            for (int i = 0; i < numOps; i++) {
                if (LLVMGetOperand(other, i) == inst) {
                    if (otherIdx > lastUse) lastUse = otherIdx;
                }
            }
        }

        live_range[inst] = std::make_pair(defIdx, lastUse);
    }
}

// find_spill: find the instruction with the longest live range endpoint
// that has a physical register assigned and overlaps with Instr
static LLVMValueRef find_spill(LLVMValueRef instr) {
    if (live_range.find(instr) == live_range.end()) return NULL;
    int instrStart = live_range[instr].first;
    int instrEnd = live_range[instr].second;

    // Build sorted list by decreasing endpoint
    std::vector<std::pair<int, LLVMValueRef>> candidates;
    for (auto& kv : live_range) {
        LLVMValueRef v = kv.first;
        if (v == instr) continue;
        int vStart = kv.second.first;
        int vEnd = kv.second.second;
        // Check overlap: ranges overlap if vStart <= instrEnd && instrStart <= vEnd
        if (vStart <= instrEnd && instrStart <= vEnd) {
            if (reg_map.find(v) != reg_map.end() && reg_map[v] != SPILLED) {
                candidates.push_back(std::make_pair(vEnd, v));
            }
        }
    }

    // Sort by decreasing endpoint
    std::sort(candidates.begin(), candidates.end(),
              [](const std::pair<int,LLVMValueRef>& a, const std::pair<int,LLVMValueRef>& b) {
                  return a.first > b.first;
              });

    if (!candidates.empty()) return candidates[0].second;
    return NULL;
}

static void allocate_registers(LLVMValueRef func) {
    reg_map.clear();

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {

        // Initialize available registers
        std::set<int> available;
        available.insert(REG_EBX);
        available.insert(REG_ECX);
        available.insert(REG_EDX);

        compute_liveness(bb);

        for (LLVMValueRef instr = LLVMGetFirstInstruction(bb);
             instr; instr = LLVMGetNextInstruction(instr)) {

            LLVMOpcode op = LLVMGetInstructionOpcode(instr);

            if (op == LLVMAlloca) continue;

            // Helper to free register of operand if its live range ends here
            auto freeEndingOperands = [&](LLVMValueRef inst) {
                int numOps = LLVMGetNumOperands(inst);
                for (int i = 0; i < numOps; i++) {
                    LLVMValueRef operand = LLVMGetOperand(inst, i);
                    if (live_range.find(operand) != live_range.end() &&
                        inst_index.find(inst) != inst_index.end()) {
                        int curIdx = inst_index[inst];
                        if (live_range[operand].second <= curIdx) {
                            if (reg_map.find(operand) != reg_map.end() && reg_map[operand] != SPILLED) {
                                available.insert(reg_map[operand]);
                            }
                        }
                    }
                }
            };

            // Check if instr produces a value
            bool producesValue = true;
            if (op == LLVMStore || op == LLVMBr || op == LLVMRet || op == LLVMUnreachable) {
                producesValue = false;
            }
            if (op == LLVMCall) {
                LLVMTypeRef retTy = LLVMTypeOf(instr);
                if (LLVMGetTypeKind(retTy) == LLVMVoidTypeKind) {
                    producesValue = false;
                }
            }

            if (!producesValue) {
                // Free registers of operands whose live ranges end here
                freeEndingOperands(instr);
                continue;
            }

            // Case 1: add/sub/mul where the first operand has a register and its range ends here
            if ((op == LLVMAdd || op == LLVMSub || op == LLVMMul)) {
                LLVMValueRef firstOp = LLVMGetOperand(instr, 0);
                if (reg_map.find(firstOp) != reg_map.end() && reg_map[firstOp] != SPILLED &&
                    live_range.find(firstOp) != live_range.end() &&
                    inst_index.find(instr) != inst_index.end() &&
                    live_range[firstOp].second <= inst_index[instr]) {

                    int R = reg_map[firstOp];
                    reg_map[instr] = R;
                    // Free second operand if its range ends
                    LLVMValueRef secondOp = LLVMGetOperand(instr, 1);
                    if (live_range.find(secondOp) != live_range.end() &&
                        live_range[secondOp].second <= inst_index[instr]) {
                        if (reg_map.find(secondOp) != reg_map.end() && reg_map[secondOp] != SPILLED) {
                            available.insert(reg_map[secondOp]);
                        }
                    }
                    continue;
                }
            }

            // Case 2: register available
            if (!available.empty()) {
                int R = *available.begin();
                available.erase(available.begin());
                reg_map[instr] = R;
                freeEndingOperands(instr);
                continue;
            }

            // Case 3: no register available, need to spill
            LLVMValueRef V = find_spill(instr);
            if (V == NULL) {
                // No one to spill, spill ourselves
                reg_map[instr] = SPILLED;
                freeEndingOperands(instr);
                continue;
            }

            // Compare: spill V or instr / spill the one with longer remaining live range
            int instrEnd = live_range.find(instr) != live_range.end() ? live_range[instr].second : 0;
            int vEnd = live_range.find(V) != live_range.end() ? live_range[V].second : 0;

            if (vEnd > instrEnd) {
                // V lives longer, so we spill V and give its register to instr
                int R = reg_map[V];
                reg_map[instr] = R;
                reg_map[V] = SPILLED;
            } else {
                // instr lives longer (or equal), spill instr
                reg_map[instr] = SPILLED;
            }
            freeEndingOperands(instr);
        }
    }
}

// Assembly code generation

static std::map<LLVMBasicBlockRef, std::string> bb_labels;
static std::map<LLVMValueRef, int> offset_map;
static int localMem;
static FILE* outFile;

static void emit(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(outFile, "\t");
    vfprintf(outFile, fmt, args);
    fprintf(outFile, "\n");
    va_end(args);
}

static void emitLabel(const char* label) {
    fprintf(outFile, "%s:\n", label);
}

static void createBBLabels(LLVMValueRef func) {
    bb_labels.clear();
    int count = 0;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        const char* name = LLVMGetBasicBlockName(bb);
        std::string label;
        if (name && name[0] != '\0') {
            label = std::string(".") + name;
        } else {
            label = ".BB" + std::to_string(count);
        }
        bb_labels[bb] = label;
        count++;
    }
}

static void printDirectives(LLVMValueRef func) {
    const char* funcName = LLVMGetValueName(func);
    fprintf(outFile, "\t.text\n");
    fprintf(outFile, "\t.globl %s\n", funcName);
    fprintf(outFile, "\t.type %s, @function\n", funcName);
    fprintf(outFile, "%s:\n", funcName);
}

static void printFunctionEnd() {
    emit("leave");
    emit("ret");
}

static void getOffsetMap(LLVMValueRef func) {
    offset_map.clear();
    localMem = 4;

    // Check if function has a parameter
    unsigned paramCount = LLVMCountParams(func);
    LLVMValueRef param = NULL;
    if (paramCount > 0) {
        param = LLVMGetParam(func, 0);
        offset_map[param] = 8;
    }

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        for (LLVMValueRef instr = LLVMGetFirstInstruction(bb);
             instr; instr = LLVMGetNextInstruction(instr)) {

            LLVMOpcode op = LLVMGetInstructionOpcode(instr);

            if (op == LLVMAlloca) {
                localMem += 4;
                offset_map[instr] = -1 * localMem;
            } else if (op == LLVMStore) {
                LLVMValueRef val = LLVMGetOperand(instr, 0);  // value
                LLVMValueRef addr = LLVMGetOperand(instr, 1);  // address

                if (param && val == param) {
                    // Store of parameter: set addr's offset to param's offset
                    if (offset_map.find(val) != offset_map.end()) {
                        offset_map[addr] = offset_map[val];
                    }
                } else if (!LLVMIsAConstantInt(val)) {
                    // Non-constant store: give val the offset of addr
                    if (offset_map.find(addr) != offset_map.end()) {
                        offset_map[val] = offset_map[addr];
                    }
                }
            } else if (op == LLVMLoad) {
                LLVMValueRef addr = LLVMGetOperand(instr, 0);
                if (offset_map.find(addr) != offset_map.end()) {
                    offset_map[instr] = offset_map[addr];
                }
            }
        }
    }
}

static bool hasPhysReg(LLVMValueRef v) {
    return reg_map.find(v) != reg_map.end() && reg_map[v] != SPILLED;
}

static bool isInMemory(LLVMValueRef v) {
    return reg_map.find(v) == reg_map.end() || reg_map[v] == SPILLED;
}

static const char* getReg(LLVMValueRef v) {
    return regNames[reg_map[v]];
}

static int getOffset(LLVMValueRef v) {
    return offset_map[v];
}

static bool isConstant(LLVMValueRef v) {
    return LLVMIsAConstantInt(v) != NULL;
}

static long long getConstVal(LLVMValueRef v) {
    return LLVMConstIntGetSExtValue(v);
}

static const char* getArithOp(LLVMOpcode op) {
    switch (op) {
        case LLVMAdd: return "addl";
        case LLVMSub: return "subl";
        case LLVMMul: return "imull";
        default: return "addl";
    }
}

static const char* getCmpJump(LLVMIntPredicate pred) {
    switch (pred) {
        case LLVMIntSLT: return "jl";
        case LLVMIntSGT: return "jg";
        case LLVMIntSLE: return "jle";
        case LLVMIntSGE: return "jge";
        case LLVMIntEQ:  return "je";
        case LLVMIntNE:  return "jne";
        default: return "je";
    }
}

static void generateCode(LLVMModuleRef module) {
    for (LLVMValueRef func = LLVMGetFirstFunction(module);
         func; func = LLVMGetNextFunction(func)) {

        // Skip declarations (extern functions like print/read)
        if (LLVMCountBasicBlocks(func) == 0) continue;

        allocate_registers(func);
        createBBLabels(func);
        printDirectives(func);
        getOffsetMap(func);

        // Prologue
        emit("pushl %%ebp");
        emit("movl %%esp, %%ebp");
        emit("subl $%d, %%esp", localMem);
        emit("pushl %%ebx");

        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
             bb; bb = LLVMGetNextBasicBlock(bb)) {

            // Print BB label
            emitLabel(bb_labels[bb].c_str());

            for (LLVMValueRef instr = LLVMGetFirstInstruction(bb);
                 instr; instr = LLVMGetNextInstruction(instr)) {

                LLVMOpcode op = LLVMGetInstructionOpcode(instr);

                if (op == LLVMAlloca) {
                    // Skip
                    continue;
                }

                if (op == LLVMRet) {
                    // ret i32 A
                    LLVMValueRef A = LLVMGetOperand(instr, 0);
                    if (isConstant(A)) {
                        emit("movl $%lld, %%eax", getConstVal(A));
                    } else if (hasPhysReg(A)) {
                        emit("movl %s, %%eax", getReg(A));
                    } else if (offset_map.find(A) != offset_map.end()) {
                        emit("movl %d(%%ebp), %%eax", getOffset(A));
                    }
                    emit("popl %%ebx");
                    printFunctionEnd();
                }

                else if (op == LLVMLoad) {
                    // %a = load i32, i32* %b
                    if (hasPhysReg(instr)) {
                        LLVMValueRef addr = LLVMGetOperand(instr, 0);
                        if (offset_map.find(addr) != offset_map.end()) {
                            emit("movl %d(%%ebp), %s", getOffset(addr), getReg(instr));
                        }
                    }
                }

                else if (op == LLVMStore) {
                    // store i32 A, i32* %b
                    LLVMValueRef A = LLVMGetOperand(instr, 0);
                    LLVMValueRef B = LLVMGetOperand(instr, 1);

                    // Check if A is the function parameter
                    unsigned paramCount = LLVMCountParams(func);
                    LLVMValueRef param = paramCount > 0 ? LLVMGetParam(func, 0) : NULL;

                    if (param && A == param) {
                        // Ignore: parameter store handled by offset_map
                    } else if (isConstant(A)) {
                        if (offset_map.find(B) != offset_map.end()) {
                            emit("movl $%lld, %d(%%ebp)", getConstVal(A), getOffset(B));
                        }
                    } else {
                        // A is a temporary variable
                        if (hasPhysReg(A)) {
                            if (offset_map.find(B) != offset_map.end()) {
                                emit("movl %s, %d(%%ebp)", getReg(A), getOffset(B));
                            }
                        } else {
                            if (offset_map.find(A) != offset_map.end() &&
                                offset_map.find(B) != offset_map.end()) {
                                emit("movl %d(%%ebp), %%eax", getOffset(A));
                                emit("movl %%eax, %d(%%ebp)", getOffset(B));
                            }
                        }
                    }
                }

                else if (op == LLVMCall) {
                    // Save caller-saved registers
                    emit("pushl %%ecx");
                    emit("pushl %%edx");

                    unsigned numArgs = LLVMGetNumArgOperands(instr);
                    if (numArgs > 0) {
                        LLVMValueRef P = LLVMGetOperand(instr, 0);
                        if (isConstant(P)) {
                            emit("pushl $%lld", getConstVal(P));
                        } else if (hasPhysReg(P)) {
                            emit("pushl %s", getReg(P));
                        } else if (offset_map.find(P) != offset_map.end()) {
                            emit("pushl %d(%%ebp)", getOffset(P));
                        }
                    }

                    // Get function name from the called function operand
                    // The called function is the last operand
                    LLVMValueRef calledFunc = LLVMGetOperand(instr, LLVMGetNumOperands(instr) - 1);
                    const char* calledName = LLVMGetValueName(calledFunc);
                    emit("call %s", calledName);

                    if (numArgs > 0) {
                        emit("addl $4, %%esp");
                    }

                    emit("popl %%edx");
                    emit("popl %%ecx");

                    // If the call returns a value
                    LLVMTypeRef retTy = LLVMTypeOf(instr);
                    if (LLVMGetTypeKind(retTy) != LLVMVoidTypeKind) {
                        if (hasPhysReg(instr)) {
                            emit("movl %%eax, %s", getReg(instr));
                        } else if (offset_map.find(instr) != offset_map.end()) {
                            emit("movl %%eax, %d(%%ebp)", getOffset(instr));
                        }
                    }
                }

                else if (op == LLVMBr) {
                    if (LLVMGetNumOperands(instr) == 1) {
                        // Unconditional: br label %b
                        LLVMBasicBlockRef target = LLVMGetSuccessor(LLVMGetBasicBlockTerminator(bb), 0);
                        emit("jmp %s", bb_labels[target].c_str());
                    } else {
                        // Conditional: br i1 %a, label %b, label %c
                        // operand 0 is the condition
                        // successor 0 is true dest, successor 1 is false dest
                        LLVMValueRef term = LLVMGetBasicBlockTerminator(bb);
                        LLVMBasicBlockRef trueDest = LLVMGetSuccessor(term, 0);
                        LLVMBasicBlockRef falseDest = LLVMGetSuccessor(term, 1);

                        LLVMValueRef cond = LLVMGetCondition(term);
                        // Get the predicate from the icmp instruction
                        LLVMIntPredicate pred = LLVMGetICmpPredicate(cond);
                        const char* jmpOp = getCmpJump(pred);

                        emit("%s %s", jmpOp, bb_labels[trueDest].c_str());
                        emit("jmp %s", bb_labels[falseDest].c_str());
                    }
                }

                else if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) {
                    // %a = add/sub/mul A, B
                    LLVMValueRef A = LLVMGetOperand(instr, 0);
                    LLVMValueRef B = LLVMGetOperand(instr, 1);
                    const char* arithOp = getArithOp(op);

                    // Determine R: register for result
                    const char* R;
                    char rBuf[8];
                    if (hasPhysReg(instr)) {
                        R = getReg(instr);
                    } else {
                        R = "%eax";
                    }

                    // Move A into R
                    if (isConstant(A)) {
                        emit("movl $%lld, %s", getConstVal(A), R);
                    } else if (hasPhysReg(A)) {
                        // Don't emit if same register
                        if (strcmp(getReg(A), R) != 0) {
                            emit("movl %s, %s", getReg(A), R);
                        }
                    } else if (offset_map.find(A) != offset_map.end()) {
                        emit("movl %d(%%ebp), %s", getOffset(A), R);
                    }

                    // Apply operation with B
                    if (isConstant(B)) {
                        emit("%s $%lld, %s", arithOp, getConstVal(B), R);
                    } else if (hasPhysReg(B)) {
                        emit("%s %s, %s", arithOp, getReg(B), R);
                    } else if (offset_map.find(B) != offset_map.end()) {
                        emit("%s %d(%%ebp), %s", arithOp, getOffset(B), R);
                    }

                    // If result is in memory, store eax
                    if (isInMemory(instr) && offset_map.find(instr) != offset_map.end()) {
                        emit("movl %%eax, %d(%%ebp)", getOffset(instr));
                    }
                }

                else if (op == LLVMICmp) {
                    // %a = icmp pred A, B
                    LLVMValueRef A = LLVMGetOperand(instr, 0);
                    LLVMValueRef B = LLVMGetOperand(instr, 1);

                    const char* R;
                    if (hasPhysReg(instr)) {
                        R = getReg(instr);
                    } else {
                        R = "%eax";
                    }

                    // Move A into R
                    if (isConstant(A)) {
                        emit("movl $%lld, %s", getConstVal(A), R);
                    } else if (hasPhysReg(A)) {
                        if (strcmp(getReg(A), R) != 0) {
                            emit("movl %s, %s", getReg(A), R);
                        }
                    } else if (offset_map.find(A) != offset_map.end()) {
                        emit("movl %d(%%ebp), %s", getOffset(A), R);
                    }

                    // Compare with B
                    if (isConstant(B)) {
                        emit("cmpl $%lld, %s", getConstVal(B), R);
                    } else if (hasPhysReg(B)) {
                        emit("cmpl %s, %s", getReg(B), R);
                    } else if (offset_map.find(B) != offset_map.end()) {
                        emit("cmpl %d(%%ebp), %s", getOffset(B), R);
                    }
                }
            }
        }
    }
}

void generateAssembly(LLVMModuleRef module, const char* outputFile) {
    outFile = fopen(outputFile, "w");
    if (!outFile) {
        fprintf(stderr, "Cannot open output file %s\n", outputFile);
        return;
    }
    generateCode(module);
    fclose(outFile);
}
