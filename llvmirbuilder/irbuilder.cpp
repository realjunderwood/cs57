#include "irbuilder.h"
#include "llvm-c/Core.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>

using namespace std;

//rename variables

static map<string, int> nameCounter;
static vector<map<string, string>> ppScopeStack;

static string getUniqueName(const string& name) {
    int id = nameCounter[name]++;
    return name + "_" + to_string(id);
}

static string lookupName(const string& name) {
    for (int i = ppScopeStack.size() - 1; i >= 0; i--) {
        auto it = ppScopeStack[i].find(name);
        if (it != ppScopeStack[i].end()) {
            return it->second;
        }
    }
    return name;
}

static void renameName(char*& field, const string& newName) {
    free(field);
    field = strdup(newName.c_str());
}

static void ppTraverseStmt(astStmt* stmt);

static void ppTraverse(astNode* node) {
    if (!node) return;

    switch (node->type) {
        case ast_prog:
            ppTraverse(node->prog.func);
            break;
        case ast_func: {
            ppScopeStack.push_back(map<string, string>());
            if (node->func.param) {
                // param is a decl node
                string orig = node->func.param->stmt.decl.name;
                string uniq = getUniqueName(orig);
                ppScopeStack.back()[orig] = uniq;
                renameName(node->func.param->stmt.decl.name, uniq);
            }
            ppTraverse(node->func.body);
            ppScopeStack.pop_back();
            break;
        }
        case ast_stmt:
            ppTraverseStmt(&node->stmt);
            break;
        case ast_var: {
            string orig = node->var.name;
            string uniq = lookupName(orig);
            renameName(node->var.name, uniq);
            break;
        }
        case ast_rexpr:
            ppTraverse(node->rexpr.lhs);
            ppTraverse(node->rexpr.rhs);
            break;
        case ast_bexpr:
            ppTraverse(node->bexpr.lhs);
            ppTraverse(node->bexpr.rhs);
            break;
        case ast_uexpr:
            ppTraverse(node->uexpr.expr);
            break;
        case ast_cnst:
        case ast_extern:
            break;
    }
}

static void ppTraverseStmt(astStmt* stmt) {
    switch (stmt->type) {
        case ast_decl: {
            string orig = stmt->decl.name;
            string uniq = getUniqueName(orig);
            ppScopeStack.back()[orig] = uniq;
            renameName(stmt->decl.name, uniq);
            break;
        }
        case ast_asgn:
            ppTraverse(stmt->asgn.lhs);
            ppTraverse(stmt->asgn.rhs);
            break;
        case ast_call:
            if (stmt->call.param)
                ppTraverse(stmt->call.param);
            break;
        case ast_ret:
            ppTraverse(stmt->ret.expr);
            break;
        case ast_while:
            ppTraverse(stmt->whilen.cond);
            ppTraverse(stmt->whilen.body);
            break;
        case ast_if:
            ppTraverse(stmt->ifn.cond);
            ppTraverse(stmt->ifn.if_body);
            if (stmt->ifn.else_body)
                ppTraverse(stmt->ifn.else_body);
            break;
        case ast_block:
            ppScopeStack.push_back(map<string, string>());
            for (astNode* s : *(stmt->block.stmt_list)) {
                ppTraverse(s);
            }
            ppScopeStack.pop_back();
            break;
    }
}

static void preprocessor(astNode* root) {
    nameCounter.clear();
    ppScopeStack.clear();
    ppTraverse(root);
}

// IR BUILDER

static map<string, LLVMValueRef> var_map;
static LLVMValueRef ret_ref;
static LLVMBasicBlockRef retBB;
static LLVMValueRef printFunc;
static LLVMValueRef readFunc;

static LLVMValueRef genIRExpr(astNode* node, LLVMBuilderRef builder);
static LLVMBasicBlockRef genIRStmt(astNode* node, LLVMBuilderRef builder, LLVMBasicBlockRef startBB);

// collect variable names in function body
static void collectVarNames(astNode* node, set<string>& names) {
    if (!node) return;
    if (node->type == ast_stmt) {
        astStmt* s = &node->stmt;
        switch (s->type) {
            case ast_decl:
                names.insert(s->decl.name);
                break;
            case ast_block:
                for (astNode* child : *(s->block.stmt_list))
                    collectVarNames(child, names);
                break;
            case ast_while:
                collectVarNames(s->whilen.body, names);
                break;
            case ast_if:
                collectVarNames(s->ifn.if_body, names);
                if (s->ifn.else_body)
                    collectVarNames(s->ifn.else_body, names);
                break;
            default:
                break;
        }
    }
}

// breadth first search from entry block to find reachable blocks and delete unreachable ones
static void removeUnreachableBlocks(LLVMValueRef func) {
    LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(func);
    set<LLVMBasicBlockRef> visited;
    queue<LLVMBasicBlockRef> worklist;
    worklist.push(entry);
    visited.insert(entry);

    while (!worklist.empty()) {
        LLVMBasicBlockRef bb = worklist.front();
        worklist.pop();
        LLVMValueRef term = LLVMGetBasicBlockTerminator(bb);
        if (!term) continue;
        unsigned n = LLVMGetNumSuccessors(term);
        for (unsigned i = 0; i < n; i++) {
            LLVMBasicBlockRef succ = LLVMGetSuccessor(term, i);
            if (visited.find(succ) == visited.end()) {
                visited.insert(succ);
                worklist.push(succ);
            }
        }
    }

    // Collect unreachable blocks
    vector<LLVMBasicBlockRef> toRemove;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func);
         bb; bb = LLVMGetNextBasicBlock(bb)) {
        if (visited.find(bb) == visited.end()) {
            toRemove.push_back(bb);
        }
    }

    // Remove unreachable blocks
    for (LLVMBasicBlockRef bb : toRemove) {
        // Remove all instructions
        LLVMValueRef inst = LLVMGetFirstInstruction(bb);
        while (inst) {
            LLVMValueRef next = LLVMGetNextInstruction(inst);
            LLVMReplaceAllUsesWith(inst, LLVMGetUndef(LLVMTypeOf(inst)));
            LLVMInstructionEraseFromParent(inst);
            inst = next;
        }
        LLVMDeleteBasicBlock(bb);
    }
}

static void generateFunction(astNode* funcNode, LLVMModuleRef module) {
    astFunc* f = &funcNode->func;

    // Determine function type
    LLVMTypeRef paramTypes[1];
    LLVMTypeRef funcType;
    bool hasParam = (f->param != NULL);

    if (hasParam) {
        paramTypes[0] = LLVMInt32Type();
        funcType = LLVMFunctionType(LLVMInt32Type(), paramTypes, 1, 0);
    } else {
        funcType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    }

    LLVMValueRef func = LLVMAddFunction(module, f->name, funcType);

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBB);

    // Collect all variable names
    set<string> varNames;
    if (hasParam) {
        varNames.insert(f->param->stmt.decl.name);
    }
    collectVarNames(f->body, varNames);

    // Create allocas for all variables
    var_map.clear();
    for (const string& name : varNames) {
        LLVMValueRef alloca = LLVMBuildAlloca(builder, LLVMInt32Type(), name.c_str());
        LLVMSetAlignment(alloca, 4);
        var_map[name] = alloca;
    }

    // Alloca for return value
    ret_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), "ret_val");
    LLVMSetAlignment(ret_ref, 4);

    // Store parameter into its alloca
    if (hasParam) {
        string paramName = f->param->stmt.decl.name;
        LLVMBuildStore(builder, LLVMGetParam(func, 0), var_map[paramName]);
    }

    // Create return basic block
    retBB = LLVMAppendBasicBlock(func, "return");
    LLVMPositionBuilderAtEnd(builder, retBB);
    LLVMValueRef retLoad = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "ret_load");
    LLVMBuildRet(builder, retLoad);

    // Generate IR for function body
    LLVMBasicBlockRef exitBB = genIRStmt(f->body, builder, entryBB);

    // If exitBB has no terminator, branch to retBB
    if (!LLVMGetBasicBlockTerminator(exitBB)) {
        LLVMPositionBuilderAtEnd(builder, exitBB);
        LLVMBuildBr(builder, retBB);
    }

    // Remove unreachable basic blocks
    removeUnreachableBlocks(func);

    LLVMDisposeBuilder(builder);
}

// genIRstmt

static LLVMBasicBlockRef genIRStmt(astNode* node, LLVMBuilderRef builder, LLVMBasicBlockRef startBB) {
    if (!node || node->type != ast_stmt) return startBB;

    astStmt* s = &node->stmt;

    switch (s->type) {
        case ast_asgn: {
            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef rhs = genIRExpr(s->asgn.rhs, builder);
            string lhsName = s->asgn.lhs->var.name;
            LLVMBuildStore(builder, rhs, var_map[lhsName]);
            return startBB;
        }
        case ast_call: {
            // print call
            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef arg = genIRExpr(s->call.param, builder);
            LLVMTypeRef paramTypes[] = { LLVMInt32Type() };
            LLVMTypeRef printType = LLVMFunctionType(LLVMVoidType(), paramTypes, 1, 0);
            LLVMValueRef args[] = { arg };
            LLVMBuildCall2(builder, printType, printFunc, args, 1, "");
            return startBB;
        }
        case ast_while: {
            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef func = LLVMGetBasicBlockParent(startBB);

            LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "while.cond");
            LLVMBuildBr(builder, condBB);

            LLVMPositionBuilderAtEnd(builder, condBB);
            LLVMValueRef cond = genIRExpr(s->whilen.cond, builder);

            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "while.body");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "while.end");
            LLVMBuildCondBr(builder, cond, trueBB, falseBB);

            LLVMBasicBlockRef trueExitBB = genIRStmt(s->whilen.body, builder, trueBB);
            if (!LLVMGetBasicBlockTerminator(trueExitBB)) {
                LLVMPositionBuilderAtEnd(builder, trueExitBB);
                LLVMBuildBr(builder, condBB);
            }

            return falseBB;
        }
        case ast_if: {
            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef func = LLVMGetBasicBlockParent(startBB);

            LLVMValueRef cond = genIRExpr(s->ifn.cond, builder);
            LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "if.then");
            LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "if.else");
            LLVMBuildCondBr(builder, cond, trueBB, falseBB);

            if (!s->ifn.else_body) {
                // there's no else
                LLVMBasicBlockRef ifExitBB = genIRStmt(s->ifn.if_body, builder, trueBB);
                if (!LLVMGetBasicBlockTerminator(ifExitBB)) {
                    LLVMPositionBuilderAtEnd(builder, ifExitBB);
                    LLVMBuildBr(builder, falseBB);
                }
                return falseBB;
            } else {
                //  there is else
                LLVMBasicBlockRef ifExitBB = genIRStmt(s->ifn.if_body, builder, trueBB);
                LLVMBasicBlockRef elseExitBB = genIRStmt(s->ifn.else_body, builder, falseBB);
                LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "if.merge");

                if (!LLVMGetBasicBlockTerminator(ifExitBB)) {
                    LLVMPositionBuilderAtEnd(builder, ifExitBB);
                    LLVMBuildBr(builder, mergeBB);
                }
                if (!LLVMGetBasicBlockTerminator(elseExitBB)) {
                    LLVMPositionBuilderAtEnd(builder, elseExitBB);
                    LLVMBuildBr(builder, mergeBB);
                }
                return mergeBB;
            }
        }
        case ast_ret: {
            LLVMPositionBuilderAtEnd(builder, startBB);
            LLVMValueRef val = genIRExpr(s->ret.expr, builder);
            LLVMBuildStore(builder, val, ret_ref);
            LLVMBuildBr(builder, retBB);

            // Create new unreachable endBB for subsequent code
            LLVMValueRef func = LLVMGetBasicBlockParent(startBB);
            LLVMBasicBlockRef endBB = LLVMAppendBasicBlock(func, "post.ret");
            return endBB;
        }
        case ast_block: {
            LLVMBasicBlockRef prevBB = startBB;
            for (astNode* child : *(s->block.stmt_list)) {
                prevBB = genIRStmt(child, builder, prevBB);
            }
            return prevBB;
        }
        case ast_decl:
            // No-op: allocas already created in entry block
            return startBB;
    }
    return startBB;
}

// genIRExpr

static LLVMValueRef genIRExpr(astNode* node, LLVMBuilderRef builder) {
    if (!node) return LLVMConstInt(LLVMInt32Type(), 0, 0);

    switch (node->type) {
        case ast_cnst:
            return LLVMConstInt(LLVMInt32Type(), node->cnst.value, 0);

        case ast_var: {
            string name = node->var.name;
            return LLVMBuildLoad2(builder, LLVMInt32Type(), var_map[name], name.c_str());
        }

        case ast_bexpr: {
            LLVMValueRef lhs = genIRExpr(node->bexpr.lhs, builder);
            LLVMValueRef rhs = genIRExpr(node->bexpr.rhs, builder);
            switch (node->bexpr.op) {
                case add:    return LLVMBuildAdd(builder, lhs, rhs, "add");
                case sub:    return LLVMBuildSub(builder, lhs, rhs, "sub");
                case mul:    return LLVMBuildMul(builder, lhs, rhs, "mul");
                case divide: return LLVMBuildSDiv(builder, lhs, rhs, "div");
                default:     return LLVMConstInt(LLVMInt32Type(), 0, 0);
            }
        }

        case ast_rexpr: {
            LLVMValueRef lhs = genIRExpr(node->rexpr.lhs, builder);
            LLVMValueRef rhs = genIRExpr(node->rexpr.rhs, builder);
            LLVMIntPredicate pred;
            switch (node->rexpr.op) {
                case lt:  pred = LLVMIntSLT; break;
                case gt:  pred = LLVMIntSGT; break;
                case le:  pred = LLVMIntSLE; break;
                case ge:  pred = LLVMIntSGE; break;
                case eq:  pred = LLVMIntEQ;  break;
                case neq: pred = LLVMIntNE;  break;
            }
            return LLVMBuildICmp(builder, pred, lhs, rhs, "cmp");
        }

        case ast_uexpr: {
            LLVMValueRef expr = genIRExpr(node->uexpr.expr, builder);
            return LLVMBuildSub(builder, LLVMConstInt(LLVMInt32Type(), 0, 0), expr, "neg");
        }

        case ast_stmt: {
            // This is a read() call: ast_stmt with type ast_call
            if (node->stmt.type == ast_call) {
                LLVMTypeRef readType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
                return LLVMBuildCall2(builder, readType, readFunc, NULL, 0, "read_val");
            }
            return LLVMConstInt(LLVMInt32Type(), 0, 0);
        }

        default:
            return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
}

// buildIR (main entry point!)

LLVMModuleRef buildIR(astNode* root) {
    // Preprocess / rename all variables to unique names
    preprocessor(root);

    // Create module
    LLVMModuleRef module = LLVMModuleCreateWithName("miniC");
    LLVMSetTarget(module, "x86_64-pc-linux-gnu");

    // Add extern function declarations
    // print
    LLVMTypeRef printParamTypes[] = { LLVMInt32Type() };
    LLVMTypeRef printType = LLVMFunctionType(LLVMVoidType(), printParamTypes, 1, 0);
    printFunc = LLVMAddFunction(module, "print", printType);

    // read
    LLVMTypeRef readType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    readFunc = LLVMAddFunction(module, "read", readType);

    // Generate  user function
    generateFunction(root->prog.func, module);

    return module;
}
