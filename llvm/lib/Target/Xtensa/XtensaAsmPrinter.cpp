//===-- XtensaAsmPrinter.cpp - Xtensa LLVM assembly writer
//------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format Xtensa assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "MCTargetDesc/XtensaTargetStreamer.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "Xtensa.h"
#include "XtensaConstantPoolValue.h"
#include "XtensaTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Constant.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace llvm;

namespace {
class XtensaAsmPrinter : public AsmPrinter {
public:
  explicit XtensaAsmPrinter(TargetMachine &TM,
                            std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "Xtensa Assembly Printer"; }

  XtensaTargetStreamer &getTargetStreamer() {
    return static_cast<XtensaTargetStreamer &>(
        *OutStreamer->getTargetStreamer());
  }

  const MCExpr *
  lowerMachineConstantPoolEntry(const XtensaConstantPoolValue *CPV);
  void emitConstantPool() override;
  void emitInstruction(const MachineInstr *MI) override;
};
} // end of anonymous namespace

const MCExpr *XtensaAsmPrinter::lowerMachineConstantPoolEntry(
    const XtensaConstantPoolValue *CPV) {
  if (auto *JumpTableCPV = dyn_cast<XtensaConstantPoolJumpTableAddr>(CPV)) {
    return MCSymbolRefExpr::create(
        GetJTISymbol(JumpTableCPV->getJumpTableIndex()), OutContext);
  }

  llvm_unreachable("unknown constant pool entry type");
}

void XtensaAsmPrinter::emitConstantPool() {
  const MachineConstantPool &MCP = *MF->getConstantPool();
  const std::vector<MachineConstantPoolEntry> &Constants = MCP.getConstants();
  if (Constants.empty()) {
    return;
  }

  XtensaTargetStreamer &Streamer = getTargetStreamer();
  Streamer.emitLiteralPosition();

  for (unsigned I = 0; I < Constants.size(); I++) {
    const MachineConstantPoolEntry &CPE = Constants[I];

    assert(CPE.getAlign() == Align(4) &&
           "Invalid xtensa constant pool entry alignment");
    assert(CPE.getSizeInBytes(getDataLayout()) == 4 &&
           "Invalid constant pool entry size");

    const MCExpr *Expr =
        CPE.isMachineConstantPoolEntry()
            ? lowerMachineConstantPoolEntry(
                  static_cast<XtensaConstantPoolValue *>(CPE.Val.MachineCPVal))
            : lowerConstant(CPE.Val.ConstVal);

    Streamer.emitLiteral(GetCPISymbol(I), Expr);
  }
}

void XtensaAsmPrinter::emitInstruction(const MachineInstr *MI) {
  Xtensa_MC::verifyInstructionPredicates(MI->getOpcode(),
                                         getSubtargetInfo().getFeatureBits());

  if (MI->getOpcode() == Xtensa::JUMPTABLE_USED) {
    // This pseudo is used just to prevent the jump table from being deleted
    // during codegen, it doesn't correspond to any actual code.
    return;
  }

  MCInst TmpInst;
  LowerXtensaMachineInstrToMCInst(MI, TmpInst, *this);
  EmitToStreamer(*OutStreamer, TmpInst);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaAsmPrinter() {
  RegisterAsmPrinter<XtensaAsmPrinter> X(getTheXtensaTarget());
}
