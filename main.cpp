#include <cstdio>
#include <cstring>
#include <string>

#include "frontend/ast.h"
#include "frontend/semanticcheck.h"
#include "llvmirbuilder/irbuilder.h"
#include "optimizations/optimizer.h"
#include "assemblygenerator/assemblygen.h"
#include "llvm-c/Core.h"

extern int yyparse();
extern int yylex_destroy();
extern FILE* yyin;
extern astNode* root;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "params incorrect");
        return 1;
    }

    // Open input file
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        return 1;
    }

    // Parse
    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed\n");
        return 1;
    }
    if (!root) {
        fprintf(stderr, "No AST produced\n");
        return 1;
    }

    // Semantic check
    if (semantic_check(root) != 0) {
        fprintf(stderr, "Semantic check failed\n");
        freeNode(root);
        return 1;
    }

    // Build IR
    LLVMModuleRef module = buildIR(root);

    // Optimize all functions
    for (LLVMValueRef fn = LLVMGetFirstFunction(module);
         fn; fn = LLVMGetNextFunction(fn)) {
        optimizeFunction(fn);
    }

    // Generate output filenames
    std::string inputName = argv[1];
    std::string llName, asmName;
    size_t dotPos = inputName.rfind('.');
    if (dotPos != std::string::npos) {
        llName = inputName.substr(0, dotPos) + ".ll";
        asmName = inputName.substr(0, dotPos) + ".s";
    } else {
        llName = inputName + ".ll";
        asmName = inputName + ".s";
    }

    // Write LLVM IR to file
    char* errMsg = nullptr;
    if (LLVMPrintModuleToFile(module, llName.c_str(), &errMsg)) {
        fprintf(stderr, "Error writing output: %s\n", errMsg);
        LLVMDisposeMessage(errMsg);
    }

    // Generate assembly
    generateAssembly(module, asmName.c_str());

    // Cleanup
    yylex_destroy();
    LLVMDisposeModule(module);
    LLVMShutdown();

    return 0;
}
