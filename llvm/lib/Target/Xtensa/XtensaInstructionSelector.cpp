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

  /// Auto-generated implementation using tablegen patterns.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;
  ComplexRendererFns selectExtuiLshrImm(MachineOperand &Root) const;

  bool selectEarly(MachineInstr &I);
  bool selectAndAsExtui(MachineInstr &I) const;
  void tryFoldShrIntoExtui(const MachineInstr &InputMI,
                           const MachineRegisterInfo &MRI, uint32_t MaskWidth,
                           Register &NewInputReg, uint32_t &ShiftWidth) const;

  bool selectLate(MachineInstr &I);
  bool selectVariableShift(MachineInstr &I, MachineBasicBlock &MBB);

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

bool isExtuiMask(uint64_t Value) { return Value <= 0xffff && isMask_64(Value); }

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

const TargetRegisterClass &XtensaInstructionSelector::getRegisterClassForReg(
    Register Reg, const MachineRegisterInfo &MRI) const {
  assert(RBI.getRegBank(Reg, MRI, TRI)->getID() == Xtensa::GPRRegBankID);
  return Xtensa::GPRRegClass;
}

bool XtensaInstructionSelector::select(MachineInstr &I) {
  if (I.getOpcode() == Xtensa::COPY) {
    MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();

    Register DstReg = I.getOperand(0).getReg();
    if (DstReg.isPhysical()) {
      return true;
    }
    return RBI.constrainGenericRegister(
        DstReg, getRegisterClassForReg(DstReg, MRI), MRI);
  }

  if (!I.isPreISelOpcode()) {
    // Already target-specific
    return true;
  }

  if (selectEarly(I)) {
    return true;
  }

  if (selectImpl(I, *CoverageInfo))
    return true;

  return selectLate(I);
}

InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectExtuiLshrImm(MachineOperand &Root) const {
  if (!Root.isReg()) {
    return None;
  }

  MachineRegisterInfo &MRI =
      Root.getParent()->getParent()->getParent()->getRegInfo();

  int64_t ShiftImm = 0;
  if (!mi_match(Root.getReg(), MRI, m_ICst(ShiftImm))) {
    return None;
  }

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

bool XtensaInstructionSelector::selectEarly(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_AND:
    return selectAndAsExtui(I);
  }
  return false;
}

bool XtensaInstructionSelector::selectAndAsExtui(MachineInstr &I) const {
  MachineBasicBlock &MBB = *I.getParent();
  MachineFunction &MF = *MBB.getParent();
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  int64_t AndImm = 0;
  Register InputReg;
  if (!mi_match(I, MRI, m_GAnd(m_Reg(InputReg), m_ICst(AndImm)))) {
    return false;
  }

  if (!isExtuiMask(AndImm)) {
    return false;
  }
  uint32_t MaskWidth = countTrailingOnes(static_cast<uint64_t>(AndImm));
  uint32_t ShiftWidth = 0;

  if (MachineInstr *InputMI = MRI.getVRegDef(InputReg)) {
    tryFoldShrIntoExtui(*InputMI, MRI, MaskWidth, InputReg, ShiftWidth);
  }

  MachineInstr *Extui = BuildMI(MBB, I, I.getDebugLoc(), TII.get(Xtensa::EXTUI))
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

void XtensaInstructionSelector::tryFoldShrIntoExtui(
    const MachineInstr &InputMI, const MachineRegisterInfo &MRI,
    uint32_t MaskWidth, Register &NewInputReg, uint32_t &ShiftWidth) const {
  unsigned InputOpcode = InputMI.getOpcode();
  if (InputOpcode != Xtensa::G_LSHR && InputOpcode != Xtensa::G_ASHR) {
    return;
  }

  int64_t ShiftImm = 0;
  if (!mi_match(InputMI.getOperand(2).getReg(), MRI, m_ICst(ShiftImm))) {
    return;
  }

  if (ShiftImm < 0 || ShiftImm > 31 || ShiftImm + MaskWidth > 32) {
    return;
  }

  NewInputReg = InputMI.getOperand(1).getReg();
  ShiftWidth = static_cast<uint32_t>(ShiftImm);
}

bool XtensaInstructionSelector::selectLate(MachineInstr &I) {
  MachineBasicBlock &MBB = *I.getParent();
  MachineFunction &MF = *MBB.getParent();

  switch (I.getOpcode()) {
  case Xtensa::G_CONSTANT: {
    // Any remaining constants at this point couldn't be folded or selected to a
    // `movi`-like instruction, so turn them into an `l32r` instead.
    const ConstantInt *Val = I.getOperand(1).getCImm();
    assert(Val->getBitWidth() == 32 &&
           "Illegal constant width, should have been legalized");

    MachineConstantPool *MCP = MF.getConstantPool();
    unsigned CPIdx = MCP->getConstantPoolIndex(Val, Align(4));
    MachineInstr *L32 = BuildMI(MBB, I, I.getDebugLoc(), TII.get(Xtensa::L32R))
                            .add(I.getOperand(0))
                            .addConstantPoolIndex(CPIdx);
    if (!constrainSelectedInstRegOperands(*L32, TII, TRI, RBI)) {
      return false;
    }
    I.eraseFromParent();
    return true;
  }
  case Xtensa::G_SHL:
  case Xtensa::G_LSHR:
  case Xtensa::G_ASHR:
    // Select to a SAR update/variable shift
    return selectVariableShift(I, MBB);
  }

  return false;
}

bool XtensaInstructionSelector::selectVariableShift(MachineInstr &I,
                                                    MachineBasicBlock &MBB) {
  bool IsLeftShift = I.getOpcode() == Xtensa::G_SHL;
  unsigned SetupOpcode = IsLeftShift ? Xtensa::SSL : Xtensa::SSR;

  unsigned ShiftOpcode = 0;
  switch (I.getOpcode()) {
  case Xtensa::G_SHL:
    ShiftOpcode = Xtensa::SLL;
    break;
  case Xtensa::G_LSHR:
    ShiftOpcode = Xtensa::SRL;
    break;
  case Xtensa::G_ASHR:
    ShiftOpcode = Xtensa::SRA;
    break;
  default:
    llvm_unreachable("Unexpected shift opcode");
  }

  MachineInstr *SetupMI = BuildMI(MBB, I, I.getDebugLoc(), TII.get(SetupOpcode))
                              .add(I.getOperand(2));
  if (!constrainSelectedInstRegOperands(*SetupMI, TII, TRI, RBI)) {
    return false;
  }

  MachineInstr *ShiftMI = BuildMI(MBB, I, I.getDebugLoc(), TII.get(ShiftOpcode))
                              .add(I.getOperand(0))
                              .add(I.getOperand(1));
  if (!constrainSelectedInstRegOperands(*ShiftMI, TII, TRI, RBI)) {
    return false;
  }

  I.removeFromParent();
  return true;
}

namespace llvm {
InstructionSelector *
createXtensaInstructionSelector(const XtensaTargetMachine &TM,
                                const XtensaSubtarget &Subtarget,
                                const XtensaRegisterBankInfo &RBI) {
  return new XtensaInstructionSelector(TM, Subtarget, RBI);
}
} // namespace llvm
