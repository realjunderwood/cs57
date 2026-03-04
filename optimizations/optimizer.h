#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "llvm-c/Core.h"

void optimizeFunction(LLVMValueRef func);

#endif
