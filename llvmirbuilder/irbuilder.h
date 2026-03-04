#ifndef IRBUILDER_H
#define IRBUILDER_H

#include "llvm-c/Core.h"
#include "../frontend/ast.h"

LLVMModuleRef buildIR(astNode* root);

#endif
