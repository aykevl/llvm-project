//===-- Xtensa.h - Top-level interface for Xtensa representation ----*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Xtensa back-end.
//
//===--------------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSA_H
#define LLVM_LIB_TARGET_XTENSA_XTENSA_H

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/PassRegistry.h"

namespace llvm {
class XtensaTargetMachine;
class FunctionPass;

FunctionPass *createXtensaISelDag(XtensaTargetMachine &TM,
                                  CodeGenOpt::Level OptLevel);
FunctionPass *createXtensaSizeReductionPass();
} // namespace llvm
#endif /* LLVM_LIB_TARGET_XTENSA_XTENSA_H */
