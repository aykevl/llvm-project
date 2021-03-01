//===-- AVRCallFrameOptimization.cpp - Optimize call frame ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that converts call frame setup instructions into
// push instructions. For example, it would convert a call frame like this:
//
//   ADJCALLSTACKDOWN 4, 0, implicit-def dead $sp, implicit-def dead $sreg, implicit $sp
//   %0:ld8 = LDIRdK 5
//   STDSPQRr $sp, 4, killed %0:ld8, implicit-def dead $sp :: (store 1 into stack + 3)
//   %1:dldregs = LDIWRdK 16416
//   STDWSPQRr $sp, 1, killed %1:dldregs, implicit-def dead $sp :: (store 2 into stack, align 1)
//
// into a serie of push instructions, like this:
//
//   ADJCALLSTACKDOWN 0, 4, implicit-def dead $sp, implicit-def dead $sreg, implicit $sp
//   %0:ld8 = LDIRdK 5
//   PUSHRr %0:ld8, implicit-def $sp, implicit $sp
//   %1:dldregs = LDIWRdK 16416
//   PUSHRr $r1, implicit-def $sp, implicit $sp
//   PUSHWRr %1:dldregs, implicit-def $sp, implicit $sp
//
// Note that there is a gap between the first and second store in the first
// example: one of the values stored is undef and thus not stored. However, when
// converting to push instructions the stack pointer still needs to be updated
// to push the instructions to the right location so a dummy "push r1"
// instruction is inserted.
//
// This pass must run before register allocation because it could increase live
// ranges in some cases.
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

#define DEBUG_NAME "AVR call frame optimization pass"

namespace {

/// Optimize AVR call frame setup instructions.
class AVRCallFrameOptimization : public MachineFunctionPass {
public:
  static char ID;

  AVRCallFrameOptimization() : MachineFunctionPass(ID) {
    initializeAVRCallFrameOptimizationPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return DEBUG_NAME; }

private:
  const AVRRegisterInfo *TRI;
  const TargetInstrInfo *TII;

  bool optimizeCallFrame(MachineInstr* MI);
  void insertExtraPushes(int numBytes, MachineInstr *At);

  /// The register that will always contain zero.
  const Register ZERO_REGISTER = AVR::R1;
};

} // end of anonymous namespace

char AVRCallFrameOptimization::ID = 0;

INITIALIZE_PASS(AVRCallFrameOptimization, "avr-cf-opt",
                DEBUG_NAME, false, false)

FunctionPass *llvm::createAVRCallFrameOptimizationPass() { return new AVRCallFrameOptimization(); }

bool AVRCallFrameOptimization::runOnMachineFunction(MachineFunction &MF) {
  const AVRSubtarget &STI = MF.getSubtarget<AVRSubtarget>();
  TRI = STI.getRegisterInfo();
  TII = STI.getInstrInfo();

  SmallVector<MachineInstr*, 4> Instrs;

  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      if (MI.getOpcode() == AVR::ADJCALLSTACKDOWN) {
        if (MI.getOperand(0).getImm() == 0)
          continue;
        Instrs.push_back(&MI);
      }
    }
  }

  bool Modified = false;
  for (auto *MI : Instrs) {
    Modified |= optimizeCallFrame(MI);
  }

  return Modified;
}

bool AVRCallFrameOptimization::optimizeCallFrame(MachineInstr *FrameStart) {
  // First scan the call frame. Start from the ADJCALLSTACKDOWN instruction
  // until we've reached the call instruction.
  MachineInstr *Call = nullptr;
  SmallVector<MachineInstr*, 8> Stores;
  for (auto *I = FrameStart->getNextNode(); I; I = I->getNextNode()) {
    if (I->isCall()) {
      Call = I;
      break;
    }
    if (I->getOpcode() == AVR::STDWSPQRr || I->getOpcode() == AVR::STDSPQRr) {
      Stores.push_back(I);
      continue;
    }
    assert(!I->modifiesRegister(AVR::SP, TRI) && "Unexpected SP modification in call frame setup!");
  }
  assert(Call && "Did not find a call in call frame setup!");

  // If the vector is sorted (which is usually the case), we can insert pushes
  // at the same location as the store instructions are now. This reduces
  // register pressure, especially for immediates as the same register can be
  // reused.
  // If it is not, fall back to inserting the push instruction at the location
  // of the last store instruction, where all to-be-stored values should be
  // live. There are more efficient ways to do this, but that's probably not
  // worth the complexity.
  bool IsSorted = llvm::is_sorted(Stores,
      [](const MachineInstr *LHS, const MachineInstr *RHS) {
    return LHS->getOperand(1).getImm() > RHS->getOperand(1).getImm();
  });
  if (!IsSorted) {
    llvm::sort(Stores, [](const MachineInstr *LHS, const MachineInstr *RHS) {
      return LHS->getOperand(1).getImm() > RHS->getOperand(1).getImm();
    });
  }

  // Replace all store instructions with push instructions.
  // This is only valid if stores are sorted.
  int64_t Offset = FrameStart->getOperand(0).getImm();
  for (auto *Store : Stores) {
    unsigned Opc;
    switch (Store->getOpcode()) {
    default:
      llvm_unreachable("Unexpected inst!");
    case AVR::STDSPQRr:
      Offset -= 1;
      Opc = AVR::PUSHRr;
      break;
    case AVR::STDWSPQRr:
      Offset -= 2;
      Opc = AVR::PUSHWRr;
      break;
    }
    auto InsertAt = Store;
    if (!IsSorted)
      // Use position of last store instruction.
      InsertAt = Stores[Stores.size()-1]->getNextNode();
    int NewOffset = (int)Store->getOperand(1).getImm() - 1;
    insertExtraPushes(Offset - NewOffset, InsertAt);
    Offset = NewOffset;
    BuildMI(*Store->getParent(), InsertAt, Store->getDebugLoc(), TII->get(Opc))
        .addReg(Store->getOperand(2).getReg());
  }

  // If the first parameter that's passed on the stack is undef, the for loop
  // above will not insert extra pushes to adjust the stack pointer. Instead,
  // some extra (dummy) pushes will be necessary to set the stack pointer.
  if (Stores.size()) {
    // Insert after the last store instruction.
    insertExtraPushes(Offset, Stores[Stores.size()-1]->getNextNode());
  } else {
    // Insert after the ADJCALLSTACKDOWN instruction, as there are no store
    // instructions.
    insertExtraPushes(Offset, FrameStart->getNextNode());
  }

  // Erase existing store instructions.
  for (auto *Store : Stores) {
    Store->eraseFromParent();
  }

  // Indicate that the ADJCALLSTACKDOWN instruction has already changed the
  // stack pointer with the 2nd (immediate) parameter of ADJCALLSTACKDOWN.
  int64_t Changed = FrameStart->getOperand(0).getImm();
  FrameStart->getOperand(0).setImm(0);
  FrameStart->getOperand(1).setImm(Changed);

  return true;
}

/// Insert extra push instructions to fill gaps in stack stores.
/// Usually, such an extra area is created when calling a function with an undef
/// parameter: the compiler is smart enough to recognize that a store is
/// unnecessary in that case. However, because we're using push instructions, we
/// have to push something there anyway to avoid an unbalanced stack.
void AVRCallFrameOptimization::insertExtraPushes(int numBytes, MachineInstr *At) {
  if (numBytes < 0) {
    // This should not happen. It means that multiple STD{W}SPQRr instructions
    // write to the same location.
    llvm_unreachable("Overlapping stack stores in frame setup!");
  }
  for (int i=0; i<numBytes; i++) {
    BuildMI(*At->getParent(), At, At->getDebugLoc(), TII->get(AVR::PUSHRr))
        .addReg(ZERO_REGISTER);
  }
}
