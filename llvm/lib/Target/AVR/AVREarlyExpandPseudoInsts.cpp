//===-- AVREarlyExpandPseudoInsts.cpp - Expand pseudo instructions --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ...
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

#include <unordered_map>

using namespace llvm;

#define AVR_EARLY_EXPAND_PSEUDO_NAME "AVR early pseudo instruction expansion pass"

namespace {

/// Expands "placeholder" instructions marked as pseudo into
/// actual AVR instructions.
class AVREarlyExpandPseudo : public MachineFunctionPass {
public:
  static char ID;

  AVREarlyExpandPseudo() : MachineFunctionPass(ID) {
    initializeAVREarlyExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return AVR_EARLY_EXPAND_PSEUDO_NAME; }

private:
  typedef MachineBasicBlock Block;
  typedef Block::iterator BlockIt;
  typedef std::unordered_map<unsigned, std::pair<Register, Register>> RegisterMap;

  const AVRRegisterInfo *TRI;
  const TargetInstrInfo *TII;
  MachineRegisterInfo *MRI;
  RegisterMap map;

  /// The register that will always contain zero.
  const Register ZERO_REGISTER = AVR::R1;

  bool expandMI(Block &MBB, BlockIt MBBI);
  template <unsigned OP> bool expand(Block &MBB, BlockIt MBBI);

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode));
  }

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode,
                              Register DstReg) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode), DstReg);
  }

  bool expandArith(unsigned OpLo, unsigned OpHi, Block &MBB, BlockIt MBBI);
  bool expandLogic(unsigned Op, Block &MBB, BlockIt MBBI);

  const TargetRegisterClass* getSmallRegClass(Register Reg);
  void splitReg(Block &MBB, BlockIt MBBI, Register Reg, Register &LoReg, Register &HiReg, unsigned &LoSubReg, unsigned &HiSubReg);
  void splitDstReg(Register DstReg, Register &DstLoReg, Register &DstHiReg);
  void joinReg(Block &MBB, BlockIt &MBBI, Register &Reg, Register &LoReg, Register &HiReg);
};

} // end of anonymous namespace

char AVREarlyExpandPseudo::ID = 0;

INITIALIZE_PASS(AVREarlyExpandPseudo, "avr-early-expand-pseudo",
                AVR_EARLY_EXPAND_PSEUDO_NAME, false, false)

FunctionPass *llvm::createAVREarlyExpandPseudoPass() { return new AVREarlyExpandPseudo(); }

bool AVREarlyExpandPseudo::
expandArith(unsigned OpLo, unsigned OpHi, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg, SrcLoReg, SrcHiReg, Src2LoReg, Src2HiReg;
  unsigned SrcLoSubReg, SrcHiSubReg, Src2LoSubReg, Src2HiSubReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  Register Src2Reg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool Src2IsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  splitDstReg(DstReg, DstLoReg, DstHiReg);
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);
  splitReg(MBB, MBBI, Src2Reg, Src2LoReg, Src2HiReg, Src2LoSubReg, Src2HiSubReg);

  buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcLoReg, 0, SrcLoSubReg)
    .addReg(Src2LoReg, 0, Src2LoSubReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg)
    .addReg(Src2HiReg, getKillRegState(Src2IsKill), Src2HiSubReg);

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

bool AVREarlyExpandPseudo::
expandLogic(unsigned Op, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg, SrcLoReg, SrcHiReg, Src2LoReg, Src2HiReg;
  unsigned SrcLoSubReg, SrcHiSubReg, Src2LoSubReg, Src2HiSubReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  Register Src2Reg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool Src2IsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  splitDstReg(DstReg, DstLoReg, DstHiReg);
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);
  splitReg(MBB, MBBI, Src2Reg, Src2LoReg, Src2HiReg, Src2LoSubReg, Src2HiSubReg);

  auto MIBLO = buildMI(MBB, MBBI, Op)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcLoReg, 0, SrcLoSubReg)
    .addReg(Src2LoReg, 0, Src2LoSubReg);

  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, Op)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg)
    .addReg(Src2HiReg, getKillRegState(Src2IsKill), Src2HiSubReg);

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::ADDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(AVR::ADDRdRr, AVR::ADCRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::ADCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(AVR::ADCRdRr, AVR::ADCRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::SUBWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(AVR::SUBRdRr, AVR::SBCRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::SBCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(AVR::SBCRdRr, AVR::SBCRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::ANDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(AVR::ANDRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::ORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(AVR::ORRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::EORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(AVR::EORRdRr, MBB, MBBI);
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::COMWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg, SrcLoReg, SrcHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  unsigned SrcLoSubReg, SrcHiSubReg;
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  splitDstReg(DstReg, DstLoReg, DstHiReg);
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);

  auto MIBLO = buildMI(MBB, MBBI, AVR::COMRd)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcLoReg, getKillRegState(SrcIsKill), SrcLoSubReg);

  // SREG is always implicitly dead
  MIBLO->getOperand(2).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, AVR::COMRd)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg);

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::NEGWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg, SrcLoReg, SrcHiReg;
  unsigned SrcLoSubReg, SrcHiSubReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  splitDstReg(DstReg, DstLoReg, DstHiReg);
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);

  Register TmpReg = MRI->createVirtualRegister(getSmallRegClass(SrcReg));

  // Do NEG on the upper byte.
  auto MIBHI =
      buildMI(MBB, MBBI, AVR::NEGRd)
          .addReg(TmpReg, RegState::Define)
          .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg);
  // SREG is always implicitly dead
  MIBHI->getOperand(2).setIsDead();

  // Do NEG on the lower byte.
  buildMI(MBB, MBBI, AVR::NEGRd)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(SrcLoReg, getKillRegState(SrcIsKill), SrcLoSubReg);

  // Do an extra SBC.
  auto MISBCI =
      buildMI(MBB, MBBI, AVR::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(TmpReg, RegState::Kill)
          .addReg(ZERO_REGISTER);
  if (ImpIsDead)
    MISBCI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MISBCI->getOperand(4).setIsKill();

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::LDIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned OpLo = AVR::LDIRdK;
  unsigned OpHi = AVR::LDIRdK;
  Register DstLoReg, DstHiReg;
  splitDstReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(1).getGlobal();
    int64_t Offs = MI.getOperand(1).getOffset();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF | AVRII::MO_LO);
    MIBHI.addGlobalAddress(GV, Offs, TF | AVRII::MO_HI);
    break;
  }
  case MachineOperand::MO_BlockAddress: {
    const BlockAddress *BA = MI.getOperand(1).getBlockAddress();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.add(MachineOperand::CreateBA(BA, TF | AVRII::MO_LO));
    MIBHI.add(MachineOperand::CreateBA(BA, TF | AVRII::MO_HI));
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(1).getImm();

    MIBLO.addImm(Imm & 0xff);
    MIBHI.addImm((Imm >> 8) & 0xff);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::STWPtrRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  unsigned SrcLoSubReg, SrcHiSubReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsUndef = MI.getOperand(0).isUndef();
  bool SrcIsKill = MI.getOperand(1).isKill();
  unsigned OpLo = AVR::STPtrRr;
  unsigned OpHi = AVR::STDPtrQRr;
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);

  //:TODO: need to reverse this order like inw and stsw?
  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstReg, getUndefRegState(DstIsUndef))
    .addReg(SrcLoReg, 0, SrcLoSubReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstReg, getUndefRegState(DstIsUndef))
    .addImm(1)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<AVR::STDWPtrQRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  unsigned SrcLoSubReg, SrcHiSubReg;
  auto Dst = MI.getOperand(0);
  Register SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(1).getImm();
  bool SrcIsKill = MI.getOperand(2).isKill();
  unsigned OpLo = AVR::STDPtrQRr;
  unsigned OpHi = AVR::STDPtrQRr;
  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .add(Dst)
    .addImm(Imm)
    .addReg(SrcLoReg, 0, SrcLoSubReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .add(Dst)
    .addImm(Imm + 1)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill), SrcHiSubReg);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <> bool AVREarlyExpandPseudo::expand<AVR::ZEXT>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  Register DstLoReg, DstHiReg;
  splitDstReg(DstReg, DstLoReg, DstHiReg);

  // Copy the lower register.
  buildMI(MBB, MBBI, TargetOpcode::COPY)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcReg);
  // Zero the higher register. Using a copy from the zero register means that
  // uses might be able to use the zero register directly.
  buildMI(MBB, MBBI, TargetOpcode::COPY)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(ZERO_REGISTER);

  joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  MI.eraseFromParent();

  return true;
}

template <>
bool AVREarlyExpandPseudo::expand<TargetOpcode::COPY>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  Register DstLoReg, DstHiReg, SrcLoReg, SrcHiReg;
  unsigned SrcLoSubReg, SrcHiSubReg;

  if (SrcReg.isPhysical()) {
    if (!AVR::DREGSRegClass.contains(SrcReg))
      return false;
  } else {
    auto SrcRegClass = MRI->getRegClass(SrcReg);
    if (TRI->getRegSizeInBits(*SrcRegClass) != 16)
      // Only split 16 bit registers.
      return false;
  }

  if (DstReg.isVirtual()) {
    if (SrcReg.isVirtual())
      return false;
    auto DstRegClass = MRI->getRegClass(DstReg);
    if (DstRegClass == &AVR::PTRREGSRegClass || DstRegClass == &AVR::PTRDISPREGSRegClass || DstRegClass == &AVR::IWREGSRegClass) {
      // These register classes are used for instructions that can directly
      // deal with these 16-bit registers. Therefore, do not expand the COPY
      // instruction so that a single movw is generated (instead of two mov
      // instructions).
      return false;
    }
    if (TRI->getRegSizeInBits(*DstRegClass) != 16)
      // Only split 16 bit registers.
      return false;
    splitDstReg(DstReg, DstLoReg, DstHiReg);
  } else {
    if (!AVR::DREGSRegClass.contains(DstReg))
      // Only split 16 bit registers.
      return false;
    TRI->splitReg(DstReg, DstLoReg, DstHiReg);
  }

  splitReg(MBB, MBBI, SrcReg, SrcLoReg, SrcHiReg, SrcLoSubReg, SrcHiSubReg);

  buildMI(MBB, MBBI, TargetOpcode::COPY, DstLoReg)
    .addReg(SrcLoReg, 0, SrcLoSubReg);
  buildMI(MBB, MBBI, TargetOpcode::COPY, DstHiReg)
    .addReg(SrcHiReg, 0, SrcHiSubReg);

  if (DstReg.isVirtual()) {
    joinReg(MBB, MBBI, DstReg, DstLoReg, DstHiReg);
  }

  MI.eraseFromParent();
  return true;
}

bool AVREarlyExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  const AVRSubtarget &STI = MF.getSubtarget<AVRSubtarget>();
  TRI = STI.getRegisterInfo();
  TII = STI.getInstrInfo();
  MRI = &MF.getRegInfo();

  bool Modified = false;

  for (Block &MBB : MF) {
    BlockIt MBBI = MBB.begin(), E = MBB.end();
    while (MBBI != E) {
      BlockIt NMBBI = std::next(MBBI);
      Modified |= expandMI(MBB, MBBI);
      MBBI = NMBBI;
    }
  }

  // Try to replace existing uses of the register pair with one of the
  // individual registers. XXX
  for (const auto&KV: map) {
    const auto &Reg = KV.first;
    const auto &Pair = KV.second;
    SmallVector<MachineOperand*, 4> Candidates;
    for (MachineOperand &MO : MRI->use_operands(Reg)) {
      if (MO.getSubReg()) {
        Candidates.push_back(&MO);
      }
    }
    for (MachineOperand *MO : Candidates) {
      unsigned SubReg = MO->getSubReg();
      if (SubReg == AVR::sub_lo) {
        MO->setReg(Pair.first);
        MO->setSubReg(0);
      } else if (SubReg == AVR::sub_hi) {
        MO->setReg(Pair.second);
        MO->setSubReg(0);
      }
    }
  }

  map.clear();
  return Modified;
}

bool AVREarlyExpandPseudo::expandMI(Block &MBB, BlockIt MBBI) {
  unsigned Opcode = MBBI->getOpcode();

#define EXPAND(Op)               \
  case Op:                       \
    return expand<Op>(MBB, MBBI)
  switch (Opcode) {
  default:
    return false;
  EXPAND(AVR::LDIWRdK);
  EXPAND(AVR::STWPtrRr);
  EXPAND(AVR::STDWPtrQRr);
  EXPAND(AVR::ADDWRdRr);
  EXPAND(AVR::ADCWRdRr);
  EXPAND(AVR::SUBWRdRr);
  EXPAND(AVR::SBCWRdRr);
  EXPAND(AVR::ANDWRdRr);
  EXPAND(AVR::ORWRdRr);
  EXPAND(AVR::EORWRdRr);
  EXPAND(AVR::COMWRd);
  EXPAND(AVR::NEGWRd);
  EXPAND(TargetOpcode::COPY);
  EXPAND(AVR::ZEXT);
  }
#undef EXPAND
}

void AVREarlyExpandPseudo::splitReg(Block &MBB, BlockIt MBBI, Register Reg, Register &LoReg, Register &HiReg, unsigned &LoSubReg, unsigned &HiSubReg) {
  LoSubReg = HiSubReg = 0;

  if (Reg.isPhysical()) {
    // Physical register, so split into (physical) subregisters.
    TRI->splitReg(Reg, LoReg, HiReg);
    return;
  }

  auto RegPairIt = map.find(Reg.id());
  if (RegPairIt != map.end()) {
    // Virtual register has been defined.
    auto RegPair = RegPairIt->second;
    LoReg = RegPair.first;
    HiReg = RegPair.second;
    return;
  }

  auto RegClass = MRI->getRegClass(Reg);
  if (RegClass != &AVR::DREGSRegClass) {
    // Register class of this register (which might be tied to the output
    // register) is not a register that can be created as an 8-bit register in
    // splitDstReg, which appears to lead to a miscompilation (I suppose the
    // register class of tied registers need to be the same).
    // TODO: only do this for tied registers and let the register class depend
    // on the instruction.
    LoReg = MRI->createVirtualRegister(&AVR::LD8RegClass);
    HiReg = MRI->createVirtualRegister(&AVR::LD8RegClass);
    buildMI(MBB, MBBI, TargetOpcode::COPY, LoReg).addReg(Reg, 0, AVR::sub_lo);
    buildMI(MBB, MBBI, TargetOpcode::COPY, HiReg).addReg(Reg, 0, AVR::sub_hi);
    return;
  }

  // Virtual register has not been defined. Return subregister indices.
  LoReg = HiReg = Reg;
  LoSubReg = AVR::sub_lo;
  HiSubReg = AVR::sub_hi;
}

const TargetRegisterClass* AVREarlyExpandPseudo::getSmallRegClass(Register Reg) {
  auto WideRegClass = MRI->getRegClass(Reg);
  if (WideRegClass == &AVR::DREGSRegClass) {
    return &AVR::GPR8RegClass;
  } else {
    return &AVR::LD8RegClass;
  }
}

void AVREarlyExpandPseudo::splitDstReg(Register DstReg, Register &DstLoReg, Register &DstHiReg) {
  auto SmallRegClass = getSmallRegClass(DstReg);
  DstLoReg = MRI->createVirtualRegister(SmallRegClass);
  DstHiReg = MRI->createVirtualRegister(SmallRegClass);
  map[DstReg.id()] = std::make_pair(DstLoReg, DstHiReg);
}

void AVREarlyExpandPseudo::joinReg(Block &MBB, BlockIt &MBBI, Register &Reg, Register &LoReg, Register &HiReg) {
  buildMI(MBB, MBBI, TargetOpcode::REG_SEQUENCE, Reg)
    .addReg(LoReg)
    .addImm(AVR::sub_lo)
    .addReg(HiReg)
    .addImm(AVR::sub_hi);
}
