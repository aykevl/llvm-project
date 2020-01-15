//===-- XtensaTargetInfo.cpp - Xtensa Target Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/TargetRegistry.h"

using namespace llvm;
namespace llvm {
Target TheXtensaTarget;
}
extern "C" void LLVMInitializeXtensaTargetInfo() {
  RegisterTarget<Triple::xtensa> X(TheXtensaTarget, "xtensa", "Xtensa 32",
                                   "XTENSA");
}
