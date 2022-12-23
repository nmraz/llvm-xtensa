//===-- XtensaInstPrinter.cpp - Convert Xtensa MCInst to assembly syntax
//-----==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints an Xtensa MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "XtensaInstPrinter.h"
#include "Xtensa.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define GET_INSTRUCTION_NAME
#include "XtensaGenAsmWriter.inc"

void XtensaInstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << getRegisterName(RegNo);
}

void XtensaInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void XtensaInstPrinter::printOperand(const MCInst *MI, int OpNum,
                                     raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);

  if (MO.isReg()) {
    printRegName(O, MO.getReg());
    return;
  }

  if (MO.isImm()) {
    O << formatImm(MO.getImm());
    return;
  }

  assert(MO.isExpr() && "Unknown operand kind in printOperand");
  MO.getExpr()->print(O, &MAI);
}

void XtensaInstPrinter::printBranchOperand(const MCInst *MI, uint64_t Address,
                                           int OpNum, raw_ostream &O) {
  printPCRelOperand<Xtensa_MC::evaluateBranchTarget>(MI, Address, OpNum, O);
}

void XtensaInstPrinter::printL32ROperand(const MCInst *MI, uint64_t Address,
                                         int OpNum, raw_ostream &O) {
  printPCRelOperand<Xtensa_MC::evaluateL32RTarget>(MI, Address, OpNum, O);
}

void XtensaInstPrinter::printCallOperand(const MCInst *MI, uint64_t Address,
                                         int OpNum, raw_ostream &O) {
  printPCRelOperand<Xtensa_MC::evaluateCallTarget>(MI, Address, OpNum, O);
}

template <uint64_t (*Evaluate)(uint64_t Addr, int64_t Imm)>
void XtensaInstPrinter::printPCRelOperand(const MCInst *MI, uint64_t Address,
                                          int OpNum, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNum);
  if (!Op.isImm()) {
    return printOperand(MI, OpNum, O);
  }

  if (PrintBranchImmAsAddress) {
    uint64_t Target = Evaluate(Address, Op.getImm());
    Target &= 0xffffffff;
    O << formatHex(Target);
  } else {
    O << formatImm(Op.getImm());
  }
}
