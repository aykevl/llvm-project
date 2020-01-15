//===-- XtensaTargetStreamer.cpp - Xtensa Target Streamer Methods---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//
//
// This file provides Xtensa specific target streamer methods.
//
//===--------------------------------------------------------------------===//

#include "XtensaTargetStreamer.h"
#include "XtensaInstPrinter.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

XtensaTargetStreamer::XtensaTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {}

XtensaTargetAsmStreamer::XtensaTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS)
    : XtensaTargetStreamer(S), OS(OS) {}

void XtensaTargetAsmStreamer::emitLiteral(std::string str) { OS << str; }

XtensaTargetELFStreamer::XtensaTargetELFStreamer(MCStreamer &S)
    : XtensaTargetStreamer(S) {}

void XtensaTargetELFStreamer::emitLiteralLabel(MCSymbol *LblSym, SMLoc L) {
  MCContext &Context = getStreamer().getContext();
  MCStreamer &OutStreamer = getStreamer();
  MCSectionELF *CS = (MCSectionELF *)OutStreamer.getCurrentSectionOnly();
  std::string CSectionName = CS->getSectionName();
  std::size_t Pos = CSectionName.find(".text");
  std::string SectionName;
  if (Pos != std::string::npos) {
    SectionName = ".literal";
    SectionName += CSectionName.substr(Pos);
  } else {
    SectionName = CSectionName;
    SectionName += ".literal";
  }

  MCSection *ConstSection = Context.getELFSection(
      SectionName, ELF::SHT_PROGBITS, ELF::SHF_EXECINSTR | ELF::SHF_ALLOC);
  ConstSection->setAlignment(Align(4));

  OutStreamer.PushSection();
  OutStreamer.SwitchSection(ConstSection);
  OutStreamer.EmitLabel(LblSym, L);
  OutStreamer.PopSection();
}

void XtensaTargetELFStreamer::emitLiteral(MCSymbol *LblSym, const MCExpr *Value,
                                          SMLoc L) {
  MCStreamer &OutStreamer = getStreamer();

  OutStreamer.EmitLabel(LblSym, L);
  OutStreamer.EmitValue(Value, 4, L);
}

void XtensaTargetELFStreamer::emitLiteral(const MCExpr *Value, SMLoc L) {
  MCContext &Context = getStreamer().getContext();
  MCStreamer &OutStreamer = getStreamer();
  MCSectionELF *CS = (MCSectionELF *)OutStreamer.getCurrentSectionOnly();
  std::string CSectionName = CS->getSectionName();
  std::size_t Pos = CSectionName.find(".text");
  std::string SectionName;
  if (Pos != std::string::npos) {
    SectionName = ".literal";
    SectionName += CSectionName.substr(Pos);
  } else {
    SectionName = CSectionName;
    SectionName += ".literal";
  }

  MCSection *ConstSection = Context.getELFSection(
      SectionName, ELF::SHT_PROGBITS, ELF::SHF_EXECINSTR | ELF::SHF_ALLOC);

  OutStreamer.PushSection();
  OutStreamer.SwitchSection(ConstSection);
  OutStreamer.EmitValue(Value, 4, L);
  OutStreamer.PopSection();
}

MCELFStreamer &XtensaTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer &>(Streamer);
}
