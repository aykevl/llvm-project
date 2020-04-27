//===- AVR.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AVR is a Harvard-architecture 8-bit micrcontroller designed for small
// baremetal programs. All AVR-family processors have 32 8-bit registers.
// The tiniest AVR has 32 byte RAM and 1 KiB program memory, and the largest
// one supports up to 2^24 data address space and 2^22 code address space.
//
// Since it is a baremetal programming, there's usually no loader to load
// ELF files on AVRs. You are expected to link your program against address
// 0 and pull out a .text section from the result using objcopy, so that you
// can write the linked code to on-chip flush memory. You can do that with
// the following commands:
//
//   ld.lld -Ttext=0 -o foo foo.o
//   objcopy -O binary --only-section=.text foo output.bin
//
// Note that the current AVR support is very preliminary so you can't
// link any useful program yet, though.
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/Object/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class AVR final : public TargetInfo {
public:
  AVR();
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

AVR::AVR() { noneRel = R_AVR_NONE; }

RelExpr AVR::getRelExpr(RelType type, const Symbol &s,
                        const uint8_t *loc) const {
  switch (type) {
  case R_AVR_7_PCREL:
  case R_AVR_13_PCREL:
    return R_PC;
  default:
    return R_ABS;
  }
}

void AVR::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_AVR_CALL: {
    // Fixup for call instructions.
    // Format: 1001 010k kkkk 111k kkkk kkkk kkkk kkkk
    checkInt(loc, val, 23, rel);
    checkAlignment(loc, val, 2, rel);
    uint16_t hi = val >> 17;
    uint16_t lo = val >> 1;
    write16le(loc, read16le(loc) | ((hi >> 1) << 4) | (hi & 1));
    write16le(loc + 2, lo);
    break;
  }
  case R_AVR_LO8_LDI:
  case R_AVR_HI8_LDI:
  case R_AVR_LO8_LDI_NEG:
  case R_AVR_HI8_LDI_NEG:
  case R_AVR_LO8_LDI_PM:
  case R_AVR_HI8_LDI_PM: {
    // Fixups for instructions such as ldi and subi. Usually two instructions
    // are used together, one that uses the lower half and one that uses the
    // upper half of a 16-bit value.
    // Format: 1110 KKKK dddd KKKK
    if (rel.type == R_AVR_LO8_LDI_NEG || rel.type == R_AVR_HI8_LDI_NEG) {
      // Relocations for subi/sbci.
      val &= 0xffff;       // clear the upper bit if this is a relocation to data space
      val = 0x10000 - val; // make negative
    }
    if (rel.type == R_AVR_LO8_LDI_PM || rel.type == R_AVR_HI8_LDI_PM)
      // Fixup for loading a function pointer, for example.
      val = val >> 1;
    if (rel.type == R_AVR_HI8_LDI || rel.type == R_AVR_HI8_LDI_PM || rel.type == R_AVR_HI8_LDI_NEG)
      // Load the upper byte (R_AVR_HI8_LDI* relocations).
      val >>= 8;
    uint16_t hi4 = (val >> 4) & 0xf;
    uint16_t lo4 = val & 0xf;
    write16le(loc, read16le(loc) | (hi4 << 8) | lo4 );
    break;
  }
  case R_AVR_7_PCREL:
    // Fixup for relative branches such as brne. The branch target (in words)
    // is k+1, so we have to subtract two from the branch target to correct for
    // that.
    // Format: 1111 01kk kkkk k001
    val = (int64_t)val - 2;
    checkInt(loc, val, 7, rel);
    checkAlignment(loc, val, 2, rel);
    write16le(loc, read16le(loc) | ((val >> 1) & 0x7f) << 3);
    break;
  case R_AVR_13_PCREL:
    // Fixup for relative jumps: rjmp and rcall. Like R_AVR_7_PCREL, the branch
    // target should be adjusted.
    // Format: 1100 kkkk kkkk kkkk
    val = (int64_t)val - 2;
    checkInt(loc, val, 13, rel);
    checkAlignment(loc, val, 2, rel);
    write16le(loc, read16le(loc) | ((val >> 1) & 0xfff));
    break;
  case R_AVR_16:
    // Note: this relocation is often used between code and data space, which
    // are 0x800000 apart in the output ELF file. The bitmask cuts off the high
    // bit.
    write16le(loc, val & 0xffff);
    break;
  case R_AVR_32:
    checkIntUInt(loc, val, 32, rel);
    write32le(loc, val);
    break;
  default:
    error(getErrorLocation(loc) + "unrecognized relocation " +
          toString(rel.type));
  }
}

TargetInfo *elf::getAVRTargetInfo() {
  static AVR target;
  return &target;
}
