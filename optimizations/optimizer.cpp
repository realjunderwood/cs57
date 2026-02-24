#include "llvm-c/Core.h"
#include "llvm-c/IRReader.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <set>
#include <unordered_map>

using StoreSet = std::set<LLVMValueRef>;

// operand 1 of store is the address, operand 0 is the value
LLVMValueRef getStoreAddr(LLVMValueRef s) {
    return LLVMGetOperand(s, 1);
}

LLVMValueRef getStoreVal(LLVMValueRef s) {
    return LLVMGetOperand(s, 0);
}

LLVMValueRef getLoadAddr(LLVMValueRef ld) {
    return LLVMGetOperand(ld, 0);
}

// if store1 and store2 are different instructions and write to the same address, store1 kills store2
bool kills(LLVMValueRef s1, LLVMValueRef s2) {
    return s1 != s2 && getStoreAddr(s1) == getStoreAddr(s2);
}

std::vector<LLVMValueRef> getAllStores(LLVMValueRef func) {
    std::vector<LLVMValueRef> res;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        for (LLVMValueRef i = LLVMGetFirstInstruction(bb);
             i; i = LLVMGetNextInstruction(i)) {
            if (LLVMGetInstructionOpcode(i) == LLVMStore)
                res.push_back(i);
        }
    }
    return res;
}

// find all predecessors of a basic block
std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>>
getpredecessors(LLVMValueRef func) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> preds;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        LLVMValueRef term = LLVMGetBasicBlockTerminator(bb);
        if (!term) continue;
        unsigned n = LLVMGetNumSuccessors(term);
        for (unsigned i = 0; i < n; i++) {
            preds[LLVMGetSuccessor(term, i)].insert(bb);
        }
    }
    return preds;
}

bool isTerminator(LLVMValueRef inst) {
    LLVMOpcode op = LLVMGetInstructionOpcode(inst);
    return op == LLVMRet || op == LLVMBr || op == LLVMSwitch ||
           op == LLVMIndirectBr || op == LLVMUnreachable;
}

// constant folding of addition, subtraction, multiplication
bool constFold(LLVMValueRef func) {
    bool changed = false;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        LLVMValueRef cur = LLVMGetFirstInstruction(bb);
        while (cur) {
            LLVMValueRef next = LLVMGetNextInstruction(cur);
            LLVMOpcode op = LLVMGetInstructionOpcode(cur);

            if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) {
                LLVMValueRef lhs = LLVMGetOperand(cur, 0);
                LLVMValueRef rhs = LLVMGetOperand(cur, 1);

                if (LLVMIsAConstantInt(lhs) && LLVMIsAConstantInt(rhs)) {
                    LLVMValueRef folded = nullptr;
                    if (op == LLVMAdd)      folded = LLVMConstAdd(lhs, rhs);
                    else if (op == LLVMSub) folded = LLVMConstSub(lhs, rhs);
                    else if (op == LLVMMul) folded = LLVMConstMul(lhs, rhs);

                    if (folded) {
                        LLVMReplaceAllUsesWith(cur, folded);
                        LLVMInstructionEraseFromParent(cur);
                        changed = true;
                    }
                }
            }
            cur = next;
        }
    }
    return changed;
}

// dead code elimination
// remove instructions with no uses (but keep any llvmstore, llvmcall,llvmalloc and terminators)
bool deadcodeelim(LLVMValueRef func) {
    bool changed = false;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        LLVMValueRef inst = LLVMGetFirstInstruction(bb);
        while (inst) {
            LLVMValueRef next = LLVMGetNextInstruction(inst);
            LLVMOpcode op = LLVMGetInstructionOpcode(inst);

            bool sideEffect = (op == LLVMStore || op == LLVMCall ||
                               op == LLVMAlloca || isTerminator(inst));

            if (!sideEffect && LLVMGetFirstUse(inst) == NULL) {
                LLVMInstructionEraseFromParent(inst);
                changed = true;
            }
            inst = next;
        }
    }
    return changed;
}

// Common subexpression elimination
bool commonsubelim(LLVMValueRef func) {
    bool changed = false;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {

        std::vector<LLVMValueRef> seen;
        LLVMValueRef inst = LLVMGetFirstInstruction(bb);

        while (inst) {
            LLVMValueRef next = LLVMGetNextInstruction(inst);
            LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);

            // skip instructions that can't be eliminated
            if (opcode == LLVMStore || opcode == LLVMCall ||
                opcode == LLVMAlloca || isTerminator(inst)) {
                seen.push_back(inst); // still track them for load safety check
                inst = next;
                continue;
            }

            int nOps = LLVMGetNumOperands(inst);

            for (auto prev : seen) {
                LLVMOpcode prev_op = LLVMGetInstructionOpcode(prev);

                if (prev_op == LLVMStore || prev_op == LLVMCall ||
                    prev_op == LLVMAlloca || isTerminator(prev))
                    continue;

                if (prev_op != opcode) continue;
                if (LLVMGetNumOperands(prev) != nOps) continue;

                // check operands match
                bool match = true;
                for (int k = 0; k < nOps; k++) {
                    if (LLVMGetOperand(inst, k) != LLVMGetOperand(prev, k)) {
                        match = false;
                        break;
                    }
                }
                if (!match) continue;

                // for loads, make sure no store to same addr in between
                if (opcode == LLVMLoad) {
                    LLVMValueRef addr = getLoadAddr(inst);
                    bool intervening = false;
                    LLVMValueRef w = LLVMGetNextInstruction(prev);
                    while (w && w != inst) {
                        if (LLVMGetInstructionOpcode(w) == LLVMStore &&
                            getStoreAddr(w) == addr) {
                            intervening = true;
                            break;
                        }
                        w = LLVMGetNextInstruction(w);
                    }
                    if (intervening) continue;
                }

                LLVMReplaceAllUsesWith(inst, prev);
                changed = true;
                break;
            }

            seen.push_back(inst);
            inst = next;
        }
    }
    return changed;
}

// CONSTANT PROPAGATION
//
// gen set: last store to each address in the block
StoreSet computeGen(LLVMBasicBlockRef bb) {
    StoreSet gen;
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb);
         inst; inst = LLVMGetNextInstruction(inst)) {
        if (LLVMGetInstructionOpcode(inst) == LLVMStore) {
            // remove stores killed by this new one
            std::vector<LLVMValueRef> dead;
            for (auto s : gen) {
                if (kills(inst, s))
                    dead.push_back(s);
            }
            for (auto s : dead)
                gen.erase(s);
            gen.insert(inst);
        }
    }
    return gen;
}

// kill set: all stores in the function that write to same addr as a store in bb
StoreSet computeKill(LLVMBasicBlockRef bb,
                             const std::vector<LLVMValueRef>& allStores) {
    StoreSet kill;
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb);
         inst; inst = LLVMGetNextInstruction(inst)) {
        if (LLVMGetInstructionOpcode(inst) == LLVMStore) {
            for (auto s : allStores) {
                if (kills(inst, s))
                    kill.insert(s);
            }
        }
    }
    return kill;
}

StoreSet setDiff(const StoreSet& a, const StoreSet& b) {
    StoreSet out;
    for (auto v : a) {
        if (b.find(v) == b.end())
            out.insert(v);
    }
    return out;
}

StoreSet setUnion(const StoreSet& a, const StoreSet& b) {
    StoreSet out = a;
    out.insert(b.begin(), b.end());
    return out;
}

bool constProp(LLVMValueRef func) {
    bool changed = false;

    std::vector<LLVMValueRef> allStores = getAllStores(func);
    if (allStores.empty()) return false;

    auto predMap = getpredecessors(func);

    std::vector<LLVMBasicBlockRef> blocks;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb))
        blocks.push_back(bb);

    std::unordered_map<LLVMBasicBlockRef, StoreSet> GEN, KILL, IN, OUT;
    for (auto bb : blocks) {
        GEN[bb] = computeGen(bb);
        KILL[bb] = computeKill(bb, allStores);
        IN[bb] = StoreSet();
        OUT[bb] = GEN[bb];
    }

    bool converged = false;
    while (!converged) {
        converged = true;
        for (auto bb : blocks) {
            // IN[B] = union of OUT[predecessors]
            StoreSet newIn;
            if (predMap.count(bb)) {
                for (auto p : predMap[bb])
                    newIn = setUnion(newIn, OUT[p]);
            }
            IN[bb] = newIn;

            // OUT[B] = GEN[B] union (IN[B] - KILL[B])
            StoreSet newOut = setUnion(GEN[bb], setDiff(IN[bb], KILL[bb]));
            if (newOut != OUT[bb]) {
                OUT[bb] = newOut;
                converged = false;
            }
        }
    }

    // now walk through each block and replace loads where all reaching stores have the same constant value
    for (auto bb : blocks) {
        StoreSet reaching = IN[bb];
        std::vector<LLVMValueRef> marked; // loads to delete after

        LLVMValueRef inst = LLVMGetFirstInstruction(bb);
        while (inst) {
            LLVMValueRef next = LLVMGetNextInstruction(inst);
            LLVMOpcode op = LLVMGetInstructionOpcode(inst);

            if (op == LLVMStore) {
                // update reaching set
                std::vector<LLVMValueRef> killed;
                for (auto s : reaching) {
                    if (kills(inst, s))
                        killed.push_back(s);
                }
                for (auto s : killed)
                    reaching.erase(s);
                reaching.insert(inst);
            } else if (op == LLVMLoad) {
                LLVMValueRef addr = getLoadAddr(inst);

                // find all stores in reaching set that write to this addr
                std::vector<LLVMValueRef> stores;
                for (auto s : reaching) {
                    if (getStoreAddr(s) == addr)
                        stores.push_back(s);
                }

                if (!stores.empty()) {
                    bool allSame = true;
                    long long cval = 0;
                    LLVMValueRef cref = nullptr;

                    for (size_t i = 0; i < stores.size(); i++) {
                        LLVMValueRef v = getStoreVal(stores[i]);
                        if (!LLVMIsAConstantInt(v)) {
                            allSame = false;
                            break;
                        }
                        long long x = LLVMConstIntGetSExtValue(v);
                        if (i == 0) {
                            cval = x;
                            cref = v;
                        } else if (x != cval) {
                            allSame = false;
                            break;
                        }
                    }

                    if (allSame && cref) {
                        // replace load with the constant, need to match type
                        LLVMTypeRef ty = LLVMTypeOf(inst);
                        LLVMValueRef replacement = LLVMConstInt(ty, cval, 1);
                        LLVMReplaceAllUsesWith(inst, replacement);
                        marked.push_back(inst);
                        changed = true;
                    }
                }
            }
            inst = next;
        }

        for (auto d : marked)
            LLVMInstructionEraseFromParent(d);
    }

    return changed;
}

void optimizeFunction(LLVMValueRef func) {
    if (LLVMCountBasicBlocks(func) == 0) return;

    // local optimizations
    commonsubelim(func);
    deadcodeelim(func);
    constFold(func);

    bool changed = true;
    while (changed) {
        changed = false;
        changed |= constProp(func);
        changed |= constFold(func);
    }

    deadcodeelim(func); // final
}

int main(int argc, char** argv) {

    LLVMMemoryBufferRef buf;
    char* msg = nullptr;
    if (LLVMCreateMemoryBufferWithContentsOfFile(argv[1], &buf, &msg)) {
        fprintf(stderr, "File read error %s \n", msg);
        LLVMDisposeMessage(msg);
        return 1;
    }

    // parse IR module
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef mod;
    if (LLVMParseIRInContext(ctx, buf, &mod, &msg)) {
        fprintf(stderr, "Error parsing IR: %s\n", msg);
        LLVMDisposeMessage(msg);
        LLVMContextDispose(ctx);
        return 1;
    }

    for (LLVMValueRef fn = LLVMGetFirstFunction(mod);
         fn; fn = LLVMGetNextFunction(fn)) {
        optimizeFunction(fn);
    }

    if (LLVMPrintModuleToFile(mod, argv[2], &msg)) {
        fprintf(stderr, "Error writing output: %s\n", msg);
        LLVMDisposeMessage(msg);
    }

    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
    return 0;
}
