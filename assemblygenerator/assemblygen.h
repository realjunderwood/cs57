#ifndef ASSEMBLYGEN_H
#define ASSEMBLYGEN_H

#include "llvm-c/Core.h"

void generateAssembly(LLVMModuleRef module, const char* outputFile);

#endif
