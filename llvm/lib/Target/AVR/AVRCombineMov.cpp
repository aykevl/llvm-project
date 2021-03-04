//===-- AVRCombineMov.cpp - Combine mov instructions ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that combines multiple mov instructions to a single
// movw instruction where possible.
//
//===----------------------------------------------------------------------===//

#include "AVR.h"
#include "AVRInstrInfo.h"
#include "AVRTargetMachine.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

using namespace llvm;

#define AVR_COMBINE_MOV_NAME "AVR combine mov instructions pass"

namespace {

/// Combines mov instructions into movw instructions.
class AVRCombineMov : public MachineFunctionPass {
public:
  static char ID;

  AVRCombineMov() : MachineFunctionPass(ID) {
    initializeAVRCombineMovPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return AVR_COMBINE_MOV_NAME; }

private:
  typedef MachineBasicBlock Block;
  typedef Block::iterator BlockIt;

  Register getOddRegister(Register Reg);
  Register getWideRegister(Register Reg);

  const AVRRegisterInfo *TRI;
  const TargetInstrInfo *TII;
};

} // end of anonymous namespace

char AVRCombineMov::ID = 0;

INITIALIZE_PASS(AVRCombineMov, "avr-combine-mov",
                AVR_COMBINE_MOV_NAME, false, false)

FunctionPass *llvm::createAVRCombineMovPass() { return new AVRCombineMov(); }

bool AVRCombineMov::runOnMachineFunction(MachineFunction &MF) {
  const AVRSubtarget &STI = MF.getSubtarget<AVRSubtarget>();
  if (!STI.hasMOVW())
    return false;

  TRI = STI.getRegisterInfo();
  TII = STI.getInstrInfo();

  bool Modified = false;
  for (Block &MBB : MF) {
    BlockIt MBBI = MBB.begin(), E = MBB.end();
    SmallVector<MachineInstr*, 4> Instrs;
    while (MBBI != E) {
      BlockIt NMBBI = std::next(MBBI);
      if (NMBBI == E)
        break;
      MachineInstr &First = *MBBI;
      MachineInstr &Second = *NMBBI;
      MBBI = NMBBI;

      // Check whether the two instructions can be combined to a movw.
      if (First.getOpcode() != AVR::MOVRdRr)
        continue;
      if (Second.getOpcode() != AVR::MOVRdRr)
        continue;
      Register DstLo = First.getOperand(0).getReg();
      Register SrcLo = First.getOperand(1).getReg();
      Register DstHi = getOddRegister(DstLo);
      Register SrcHi = getOddRegister(SrcLo);
      if (DstHi == 0 || SrcHi == 0)
        // DstLo or SrcLo are not an even GPR8 register.
        continue;
      if (Second.getOperand(0).getReg() != DstHi
          || Second.getOperand(1).getReg() != SrcHi)
        // The second mov instruction does not have the right operands to merge
        // them into a movw instruction.
        continue;

      // All preconditions are satisfied. It is possible to replace these two
      // instructions with a single movw instruction, so do it now.
      bool DstIsKill = First.getOperand(1).isKill()
          && Second.getOperand(1).isKill();
      Register Dst = getWideRegister(DstLo);
      Register Src = getWideRegister(SrcLo);
      BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(AVR::MOVWRdRr))
          .addReg(Dst, RegState::Define)
          .addReg(Src, getKillRegState(DstIsKill));
      MBBI = std::next(MBBI); // skip over second mov instruction
      Modified |= true;
      First.eraseFromParent();
      Second.eraseFromParent();
    }
  }

  return Modified;
}

// Return odd GPR8 register for an even GPR8 register. Returns 0 if Reg is not
// an even register.
Register AVRCombineMov::getOddRegister(Register Reg) {
  switch (Reg) {
  default:
    // not an even GPR8 register
    return 0;
  case AVR::R0:  return AVR::R1;
  case AVR::R2:  return AVR::R3;
  case AVR::R4:  return AVR::R5;
  case AVR::R6:  return AVR::R7;
  case AVR::R8:  return AVR::R9;
  case AVR::R10: return AVR::R11;
  case AVR::R12: return AVR::R13;
  case AVR::R14: return AVR::R15;
  case AVR::R16: return AVR::R17;
  case AVR::R18: return AVR::R19;
  case AVR::R20: return AVR::R21;
  case AVR::R22: return AVR::R23;
  case AVR::R24: return AVR::R25;
  case AVR::R26: return AVR::R27;
  case AVR::R28: return AVR::R29;
  case AVR::R30: return AVR::R31;
  }
}

// Return the wide register for an even GPR8 register.
Register AVRCombineMov::getWideRegister(Register Reg) {
  switch (Reg) {
  default:
    // not an even GPR8 register
    return 0;
  case AVR::R0:  return AVR::R1R0;
  case AVR::R2:  return AVR::R3R2;
  case AVR::R4:  return AVR::R5R4;
  case AVR::R6:  return AVR::R7R6;
  case AVR::R8:  return AVR::R9R8;
  case AVR::R10: return AVR::R11R10;
  case AVR::R12: return AVR::R13R12;
  case AVR::R14: return AVR::R15R14;
  case AVR::R16: return AVR::R17R16;
  case AVR::R18: return AVR::R19R18;
  case AVR::R20: return AVR::R21R20;
  case AVR::R22: return AVR::R23R22;
  case AVR::R24: return AVR::R25R24;
  case AVR::R26: return AVR::R27R26;
  case AVR::R28: return AVR::R29R28;
  case AVR::R30: return AVR::R31R30;
  }
}
