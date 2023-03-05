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
#include "XtensaISelUtils.h"
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
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LowLevelTypeImpl.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-isel"

using namespace llvm;
using namespace MIPatternMatch;
using namespace XtensaISelUtils;

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

  Register createVirtualGPR() const;

  MachineInstrBuilder emitInstrFor(MachineInstr &I, unsigned Opcode) const;
  bool emitCopy(MachineInstr &I, Register Dest, Register Src);
  bool emitL32R(MachineInstr &I, Register Dest, const Constant *Value);

  bool selectCopy(MachineInstr &I);

  bool preISelLower(MachineInstr &I);
  bool convertPtrAddToAdd(MachineInstr &I);

  /// Auto-generated implementation using tablegen patterns.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  ComplexRendererFns selectExtuiLshrImm(MachineOperand &Root) const;
  template <unsigned W, unsigned S>
  ComplexRendererFns selectPtrOff(MachineOperand &Root) const;

  bool selectEarly(MachineInstr &I);
  bool selectAndAsExtui(MachineInstr &I) const;
  void tryFoldShrIntoExtui(const MachineInstr &InputMI,
                           const MachineRegisterInfo &MRI, uint32_t MaskWidth,
                           Register &NewInputReg, uint32_t &ShiftWidth) const;
  bool selectAddSubConst(MachineInstr &I);
  void emitAddParts(MachineInstr &I, Register Dest, Register Operand,
                    const AddConstParts &Parts);

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

static bool isExtuiMask(uint64_t Value) {
  return Value <= 0xffff && isMask_64(Value);
}

bool XtensaInstructionSelector::select(MachineInstr &I) {
  if (I.getOpcode() == Xtensa::COPY) {
    return selectCopy(I);
  }

  if (!I.isPreISelOpcode()) {
    // Already target-specific
    return true;
  }

  if (!preISelLower(I)) {
    return false;
  }

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

Register XtensaInstructionSelector::createVirtualGPR() const {
  MachineRegisterInfo &MRI = MF->getRegInfo();
  Register Reg = MRI.createGenericVirtualRegister(LLT::scalar(32));
  MRI.setRegBank(Reg, RBI.getRegBank(Xtensa::GPRRegBankID));
  return Reg;
}

MachineInstrBuilder
XtensaInstructionSelector::emitInstrFor(MachineInstr &I,
                                        unsigned Opcode) const {
  return BuildMI(*CurMBB, I, I.getDebugLoc(), TII.get(Opcode));
}

bool XtensaInstructionSelector::emitL32R(MachineInstr &I, Register Dest,
                                         const Constant *Value) {
  MachineConstantPool *MCP = MF->getConstantPool();

  Align Alignment = Align(4);
  unsigned CPIdx = MCP->getConstantPoolIndex(Value, Alignment);
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getConstantPool(*MF), MachineMemOperand::MOLoad,
      LLT::scalar(32), Alignment);

  MachineInstr *L32 = emitInstrFor(I, Xtensa::L32R)
                          .addDef(Dest)
                          .addConstantPoolIndex(CPIdx)
                          .addMemOperand(MMO);
  return constrainSelectedInstRegOperands(*L32, TII, TRI, RBI);
}

bool XtensaInstructionSelector::emitCopy(MachineInstr &I, Register Dest,
                                         Register Src) {
  MachineInstr *CopyInst =
      emitInstrFor(I, Xtensa::COPY).addDef(Dest).addReg(Src);
  return selectCopy(*CopyInst);
}

bool XtensaInstructionSelector::selectCopy(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

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

bool XtensaInstructionSelector::preISelLower(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_PTR_ADD:
    return convertPtrAddToAdd(I);
  }

  return true;
}

bool XtensaInstructionSelector::convertPtrAddToAdd(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  // If this instruction still exists at this point, we haven't been able to
  // fold it into a neighboring load or store. Lower it to a regular add so the
  // existing patterns can match.

  // Emit a pointer-to-integer copy for the source operand. We don't need to do
  // something analagous for the destination operand as uses are always selected
  // before defs, so we can change its type directly.
  Register IntReg = createVirtualGPR();
  if (!emitCopy(I, IntReg, I.getOperand(1).getReg())) {
    return false;
  }

  I.setDesc(TII.get(Xtensa::G_ADD));
  MRI.setType(I.getOperand(0).getReg(), LLT::scalar(32));
  I.getOperand(1).setReg(IntReg);

  // Fold in negation of the offset to create a `G_SUB`
  Register OffReg = I.getOperand(2).getReg();
  Register NegOffReg;
  if (mi_match(OffReg, MRI, m_GSub(m_SpecificICst(0), m_Reg(NegOffReg)))) {
    I.setDesc(TII.get(Xtensa::G_SUB));
    I.getOperand(2).setReg(NegOffReg);
  }

  return true;
}

InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectExtuiLshrImm(MachineOperand &Root) const {
  if (!Root.isReg()) {
    return None;
  }

  MachineRegisterInfo &MRI = MF->getRegInfo();

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

template <unsigned W, unsigned S>
InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectPtrOff(MachineOperand &Root) const {
  const MachineRegisterInfo &MRI = MF->getRegInfo();

  Register Base;
  int64_t Offset = 0;
  if (!mi_match(Root.getReg(), MRI, m_GPtrAdd(m_Reg(Base), m_ICst(Offset)))) {
    return None;
  }

  if (!isShiftedUInt<W, S>(Offset)) {
    return None;
  }

  return {{
      [=](MachineInstrBuilder &MIB) { MIB.addReg(Base); },
      [=](MachineInstrBuilder &MIB) { MIB.addImm(Offset); },
  }};
}

bool XtensaInstructionSelector::selectEarly(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  switch (I.getOpcode()) {
  case Xtensa::G_PHI: {
    I.setDesc(TII.get(Xtensa::PHI));
    Register DstReg = I.getOperand(0).getReg();
    return RBI.constrainGenericRegister(
        DstReg, getRegisterClassForReg(DstReg, MRI), MRI);
  }
  case Xtensa::G_AND:
    return selectAndAsExtui(I);
  case Xtensa::G_ADD:
  case Xtensa::G_SUB:
    return selectAddSubConst(I);
  case Xtensa::G_PTRTOINT:
  case Xtensa::G_INTTOPTR:
    return selectCopy(I);
  }

  return false;
}

bool XtensaInstructionSelector::selectAndAsExtui(MachineInstr &I) const {
  const MachineRegisterInfo &MRI = MF->getRegInfo();

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

void XtensaInstructionSelector::tryFoldShrIntoExtui(
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

bool XtensaInstructionSelector::selectAddSubConst(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  unsigned Opcode = I.getOpcode();
  assert(Opcode == Xtensa::G_ADD || Opcode == Xtensa::G_SUB);

  bool IsSub = Opcode == Xtensa::G_SUB;

  Register Dest = I.getOperand(0).getReg();
  Register LHS = I.getOperand(1).getReg();
  Register RHS = I.getOperand(2).getReg();

  // Start by trying to match a constant RHS, which works for both addition and
  // subtraction.
  Register ConstOperand = RHS;
  Register OtherOperand = LHS;
  Optional<int64_t> MaybeConstValue =
      getIConstantVRegSExtVal(ConstOperand, MRI);

  // For addition, a constant LHS can also be optimized.
  if (!MaybeConstValue && !IsSub) {
    ConstOperand = LHS;
    OtherOperand = RHS;
    MaybeConstValue = getIConstantVRegSExtVal(ConstOperand, MRI);
  }

  if (!MaybeConstValue) {
    return false;
  }

  int64_t ConstValue = *MaybeConstValue;
  if (IsSub) {
    ConstValue = -ConstValue;
  }

  Optional<AddConstParts> Parts = splitAddConst(ConstValue);
  if (!Parts) {
    return false;
  }

  // Heuristic: this is a big constant, so if it is used elsewhere and we would
  // need two instructions anyway, it's probably better to just let the original
  // constant be materialized.
  if (Parts->High && Parts->Low && !MRI.hasOneNonDBGUse(ConstOperand)) {
    return false;
  }

  emitAddParts(I, Dest, OtherOperand, *Parts);
  I.removeFromParent();
  return true;
}

void XtensaInstructionSelector::emitAddParts(MachineInstr &I, Register Dest,
                                             Register Operand,
                                             const AddConstParts &Parts) {
  Register AddmiDest = Dest;
  Register AddiSrc = Operand;

  if (!Parts.High && !Parts.Low) {
    // Make sure adding 0 still emits at least something.
    emitCopy(I, Dest, Operand);
    return;
  }

  if (Parts.High && Parts.Low) {
    // We need two opcodes, so create a new temporary register.
    Register TempReg = createVirtualGPR();
    AddmiDest = TempReg;
    AddiSrc = TempReg;
  }

  if (Parts.High) {
    MachineInstr *AddMi = emitInstrFor(I, Xtensa::ADDMI)
                              .addDef(AddmiDest)
                              .addReg(Operand)
                              .addImm(Parts.High << 8);
    constrainSelectedInstRegOperands(*AddMi, TII, TRI, RBI);
  }

  if (Parts.Low) {
    MachineInstr *Addi = emitInstrFor(I, Xtensa::ADDI)
                             .addDef(Dest)
                             .addReg(AddiSrc)
                             .addImm(Parts.Low);
    constrainSelectedInstRegOperands(*Addi, TII, TRI, RBI);
  }
}

bool XtensaInstructionSelector::selectLate(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_CONSTANT: {
    // Any remaining constants at this point couldn't be folded or selected to a
    // `movi`-like instruction, so turn them into an `l32r` instead.
    const ConstantInt *Val = I.getOperand(1).getCImm();
    assert(Val->getBitWidth() == 32 &&
           "Illegal constant width, should have been legalized");

    if (!emitL32R(I, I.getOperand(0).getReg(), Val)) {
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
