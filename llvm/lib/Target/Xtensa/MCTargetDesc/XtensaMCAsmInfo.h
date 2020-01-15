//===-- XtensaMCAsmInfo.h - Xtensa Asm Info --------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the XtensaMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSATARGETASMINFO_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSATARGETASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class XtensaMCAsmInfo : public MCAsmInfoELF {
public:
  explicit XtensaMCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif /* LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSATARGETASMINFO_H */
