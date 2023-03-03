//===- XtensaInstructionSelector.cpp -----------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the InstructionSelector class for
/// Xtensa.
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaRegisterBankInfo.h"
#include "XtensaRegisterInfo.h"
#include "XtensaSubtarget.h"
#include "XtensaTargetMachine.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-isel"

using namespace llvm;
using namespace MIPatternMatch;

namespace {

#define GET_GLOBALISEL_PREDICATE_BITSET
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATE_BITSET

class XtensaInstructionSelector : public InstructionSelector {
public:
  XtensaInstructionSelector(const XtensaTargetMachine &TM,
                            const XtensaSubtarget &STI,
                            const XtensaRegisterBankInfo &RBI);

  bool select(MachineInstr &I) override;
  static const char *getName() { return DEBUG_TYPE; }

private:
  const TargetRegisterClass &
  getRegisterClassForReg(Register Reg, const MachineRegisterInfo &MRI) const;

  MachineInstrBuilder emitInstrFor(MachineInstr &I, unsigned Opcode) const;

  bool selectCOPY(MachineInstr &I, MachineRegisterInfo &MRI);
  void emitCOPY(MachineInstr &I, Register Dest, Register Src);

  void preISelLower(MachineInstr &I);
  void convertPtrAddToAdd(MachineInstr &I);

  /// Auto-generated implementation using tablegen patterns.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  ComplexRendererFns selectEXTUILshrImm(MachineOperand &Root) const;
  void renderNegImm(MachineInstrBuilder &MIB, const MachineInstr &I,
                    int OpIdx = -1) const;

  bool selectEarly(MachineInstr &I);
  bool selectANDAsEXTUI(MachineInstr &I) const;
  void tryFoldShrIntoEXTUI(const MachineInstr &InputMI,
                           const MachineRegisterInfo &MRI, uint32_t MaskWidth,
                           Register &NewInputReg, uint32_t &ShiftWidth) const;

  bool selectLate(MachineInstr &I);

  const XtensaInstrInfo &TII;
  const XtensaRegisterInfo &TRI;
  const XtensaRegisterBankInfo &RBI;

#define GET_GLOBALISEL_PREDICATES_DECL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_DECL

#define GET_GLOBALISEL_TEMPORARIES_DECL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_DECL
};

} // end anonymous namespace

#define GET_GLOBALISEL_IMPL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_IMPL

XtensaInstructionSelector::XtensaInstructionSelector(
    const XtensaTargetMachine &TM, const XtensaSubtarget &STI,
    const XtensaRegisterBankInfo &RBI)
    : TII(*STI.getInstrInfo()), TRI(*STI.getRegisterInfo()), RBI(RBI),
#define GET_GLOBALISEL_PREDICATES_INIT
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_INIT
#define GET_GLOBALISEL_TEMPORARIES_INIT
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_INIT
{
}

static bool isEXTUIMask(uint64_t Value) {
  return Value <= 0xffff && isMask_64(Value);
}

bool XtensaInstructionSelector::select(MachineInstr &I) {
  if (I.getOpcode() == Xtensa::COPY) {
    MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();
    return selectCOPY(I, MRI);
  }

  if (!I.isPreISelOpcode()) {
    // Already target-specific
    return true;
  }

  preISelLower(I);

  if (selectEarly(I)) {
    return true;
  }

  if (selectImpl(I, *CoverageInfo))
    return true;

  return selectLate(I);
}

const TargetRegisterClass &XtensaInstructionSelector::getRegisterClassForReg(
    Register Reg, const MachineRegisterInfo &MRI) const {
  assert(RBI.getRegBank(Reg, MRI, TRI)->getID() == Xtensa::GPRRegBankID);
  return Xtensa::GPRRegClass;
}

MachineInstrBuilder
XtensaInstructionSelector::emitInstrFor(MachineInstr &I,
                                        unsigned Opcode) const {
  return BuildMI(*CurMBB, I, I.getDebugLoc(), TII.get(Opcode));
}

bool XtensaInstructionSelector::selectCOPY(MachineInstr &I,
                                           MachineRegisterInfo &MRI) {
  I.setDesc(TII.get(Xtensa::COPY));
  for (MachineOperand &Op : I.operands()) {
    Register Reg = Op.getReg();
    if (Reg.isPhysical()) {
      continue;
    }

    if (!RBI.constrainGenericRegister(Reg, getRegisterClassForReg(Reg, MRI),
                                      MRI)) {
      return false;
    }
  }

  return true;
}

void XtensaInstructionSelector::emitCOPY(MachineInstr &I, Register Dest,
                                         Register Src) {
  MachineInstr *CopyInst =
      emitInstrFor(I, Xtensa::COPY).addDef(Dest).addReg(Src);
  if (!selectCOPY(*CopyInst, MF->getRegInfo())) {
    llvm_unreachable("Failed to emit a copy");
  }
}

void XtensaInstructionSelector::preISelLower(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_PTR_ADD:
    convertPtrAddToAdd(I);
    break;
  }
}

void XtensaInstructionSelector::convertPtrAddToAdd(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();
  LLT S32 = LLT::scalar(32);

  // If this instruction still exists at this point, we haven't been able to
  // fold it into a neighboring load or store. Lower it to a regular add so the
  // existing patterns can match.

  // Emit a pointer-to-integer copy for the source operand. We don't need to do
  // something analagous for the destination operand as uses are always selected
  // before defs, so we can change its type directly.
  Register IntReg = MRI.createGenericVirtualRegister(S32);
  MRI.setRegBank(IntReg, RBI.getRegBank(Xtensa::GPRRegBankID));
  emitCOPY(I, IntReg, I.getOperand(1).getReg());

  I.setDesc(TII.get(Xtensa::G_ADD));
  MRI.setType(I.getOperand(0).getReg(), S32);
  I.getOperand(1).setReg(IntReg);
}

InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectEXTUILshrImm(MachineOperand &Root) const {
  if (!Root.isReg()) {
    return None;
  }

  MachineRegisterInfo &MRI =
      Root.getParent()->getParent()->getParent()->getRegInfo();

  Optional<int64_t> MaybeShiftImm = getIConstantVRegSExtVal(Root.getReg(), MRI);
  if (!MaybeShiftImm) {
    return None;
  }

  int64_t ShiftImm = *MaybeShiftImm;
  if (ShiftImm < 16 || ShiftImm > 31) {
    return None;
  }

  return {{
      // Shift amount
      [=](MachineInstrBuilder &MIB) { MIB.addImm(ShiftImm); },
      // Mask size
      [=](MachineInstrBuilder &MIB) { MIB.addImm(32 - ShiftImm); },
  }};
}

void XtensaInstructionSelector::renderNegImm(MachineInstrBuilder &MIB,
                                             const MachineInstr &I,
                                             int OpIdx) const {
  assert(OpIdx == -1);
  int64_t Val = I.getOperand(1).getCImm()->getSExtValue();
  MIB.addImm(-Val);
}

bool XtensaInstructionSelector::selectEarly(MachineInstr &I) {
  MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();

  switch (I.getOpcode()) {
  case Xtensa::G_PHI: {
    I.setDesc(TII.get(Xtensa::PHI));
    Register DstReg = I.getOperand(0).getReg();
    return RBI.constrainGenericRegister(
        DstReg, getRegisterClassForReg(DstReg, MRI), MRI);
  }
  case Xtensa::G_AND:
    return selectANDAsEXTUI(I);
  case Xtensa::G_PTRTOINT:
  case Xtensa::G_INTTOPTR:
    return selectCOPY(I, MRI);
  }

  return false;
}

bool XtensaInstructionSelector::selectANDAsEXTUI(MachineInstr &I) const {
  const MachineRegisterInfo &MRI = MF->getRegInfo();

  int64_t AndImm = 0;
  Register InputReg;
  if (!mi_match(I, MRI, m_GAnd(m_Reg(InputReg), m_ICst(AndImm)))) {
    return false;
  }

  if (!isEXTUIMask(AndImm)) {
    return false;
  }
  uint32_t MaskWidth = countTrailingOnes(static_cast<uint64_t>(AndImm));
  uint32_t ShiftWidth = 0;

  if (MachineInstr *InputMI = MRI.getVRegDef(InputReg)) {
    tryFoldShrIntoEXTUI(*InputMI, MRI, MaskWidth, InputReg, ShiftWidth);
  }

  MachineInstr *Extui = emitInstrFor(I, Xtensa::EXTUI)
                            .add(I.getOperand(0))
                            .addReg(InputReg)
                            .addImm(ShiftWidth)
                            .addImm(MaskWidth);
  if (!constrainSelectedInstRegOperands(*Extui, TII, TRI, RBI)) {
    return false;
  }

  I.removeFromParent();
  return true;
}

void XtensaInstructionSelector::tryFoldShrIntoEXTUI(
    const MachineInstr &InputMI, const MachineRegisterInfo &MRI,
    uint32_t MaskWidth, Register &NewInputReg, uint32_t &ShiftWidth) const {
  unsigned InputOpcode = InputMI.getOpcode();
  if (InputOpcode != Xtensa::G_LSHR && InputOpcode != Xtensa::G_ASHR) {
    return;
  }

  Optional<int64_t> MaybeShiftImm =
      getIConstantVRegSExtVal(InputMI.getOperand(2).getReg(), MRI);
  if (!MaybeShiftImm) {
    return;
  }

  int64_t ShiftImm = *MaybeShiftImm;
  if (ShiftImm < 0 || ShiftImm > 31 || ShiftImm + MaskWidth > 32) {
    return;
  }

  NewInputReg = InputMI.getOperand(1).getReg();
  ShiftWidth = static_cast<uint32_t>(ShiftImm);
}

bool XtensaInstructionSelector::selectLate(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_CONSTANT: {
    // Any remaining constants at this point couldn't be folded or selected to a
    // `movi`-like instruction, so turn them into an `l32r` instead.
    const ConstantInt *Val = I.getOperand(1).getCImm();
    assert(Val->getBitWidth() == 32 &&
           "Illegal constant width, should have been legalized");

    MachineConstantPool *MCP = MF->getConstantPool();
    unsigned CPIdx = MCP->getConstantPoolIndex(Val, Align(4));
    MachineInstr *L32 = emitInstrFor(I, Xtensa::L32R)
                            .add(I.getOperand(0))
                            .addConstantPoolIndex(CPIdx);
    if (!constrainSelectedInstRegOperands(*L32, TII, TRI, RBI)) {
      return false;
    }
    I.eraseFromParent();
    return true;
  }
  case Xtensa::G_XTENSA_SSR_MASKED:
  case Xtensa::G_XTENSA_SSR_INRANGE:
    // We mutate the opcode manually due to a tablegen bug with implicit defs:
    // the opcode mutation logic currently doesn't take any implicit defs in the
    // source instruction into account, and so always manually adds an extra
    // `SAR` def.
    I.setDesc(TII.get(Xtensa::SSR));
    return constrainSelectedInstRegOperands(I, TII, TRI, RBI);
  case Xtensa::G_XTENSA_SSL_MASKED:
  case Xtensa::G_XTENSA_SSL_INRANGE:
    I.setDesc(TII.get(Xtensa::SSL));
    return constrainSelectedInstRegOperands(I, TII, TRI, RBI);
  }

  return false;
}

namespace llvm {
InstructionSelector *
createXtensaInstructionSelector(const XtensaTargetMachine &TM,
                                const XtensaSubtarget &Subtarget,
                                const XtensaRegisterBankInfo &RBI) {
  return new XtensaInstructionSelector(TM, Subtarget, RBI);
}
} // namespace llvm
