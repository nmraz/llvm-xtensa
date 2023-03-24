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

#include "MCTargetDesc/XtensaBaseInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaInstrUtils.h"
#include "XtensaRegisterBankInfo.h"
#include "XtensaRegisterInfo.h"
#include "XtensaSubtarget.h"
#include "XtensaTargetMachine.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
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
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LowLevelTypeImpl.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <utility>

#define DEBUG_TYPE "xtensa-isel"

using namespace llvm;
using namespace MIPatternMatch;
using namespace XtensaInstrUtils;
using namespace XtensaII;

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
  void constrainInstrRegisters(MachineInstr &MI) const;
  bool forceConstrainInstrRegisters(MachineInstr &MI);

  Register createVirtualGPR() const;

  MachineInstrBuilder emitInstrFor(MachineInstr &I, unsigned Opcode) const;
  bool emitCopy(MachineInstr &I, Register Dest, Register Src);
  bool emitL32R(MachineInstr &I, Register Dest, const Constant *Value);
  void emitAddParts(MachineInstr &I, Register Dest, Register Operand,
                    const AddConstParts &Parts);
  bool emitICmpSelect(MachineInstr &I, CmpInst::Predicate Pred, Register CmpLHS,
                      Register CmpRHS, Register TrueVal, Register FalseVal);

  bool preISelLower(MachineInstr &I);
  bool convertPtrAddToAdd(MachineInstr &I);
  void convertPtrLoad(MachineInstr &I);
  bool convertPtrStore(MachineInstr &I);

  /// Auto-generated implementation using tablegen patterns.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  ComplexRendererFns selectExtuiLshrImm(MachineOperand &Root) const;
  ComplexRendererFns selectFrameIndexOff(MachineOperand &Root) const;

  void renderImmPlus1(MachineInstrBuilder &MIB, const MachineInstr &I,
                      int OpIdx = -1) const;
  void renderBittestMask(MachineInstrBuilder &MIB, const MachineInstr &I,
                         int OpIdx = -1) const;

  bool selectEarly(MachineInstr &I);
  bool selectAndAsExtui(MachineInstr &I) const;
  void tryFoldShrIntoExtui(const MachineInstr &InputMI,
                           const MachineRegisterInfo &MRI, uint32_t MaskWidth,
                           Register &NewInputReg, uint32_t &ShiftWidth) const;
  bool selectSelectFedByICmp(MachineInstr &I);
  bool selectSextInreg(MachineInstr &I);
  bool selectAddSubConst(MachineInstr &I);
  bool selectFrameIndexOffset(MachineInstr &I, MachineInstr &OperandMI,
                              int64_t Offset);

  bool selectLate(MachineInstr &I);
  bool selectICmp(MachineInstr &I);
  bool selectLoadStore(MachineInstr &I);

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

static Optional<unsigned> getLoadOpcode(unsigned ByteSize) {
  switch (ByteSize) {
  case 1:
    return Xtensa::L8UI;
  case 2:
    return Xtensa::L16UI;
  case 4:
    return Xtensa::L32I;
  }
  return None;
}

static Optional<unsigned> getStoreOpcode(unsigned ByteSize) {
  switch (ByteSize) {
  case 1:
    return Xtensa::S8I;
  case 2:
    return Xtensa::S16I;
  case 4:
    return Xtensa::S32I;
  }
  return None;
}

static Optional<unsigned> getSextLoadOpcode(unsigned ByteSize) {
  switch (ByteSize) {
  case 2:
    return Xtensa::L16SI;
  }
  return None;
}

static Optional<unsigned> getLoadStoreOpcode(unsigned Opcode,
                                             uint64_t ByteSize) {
  switch (Opcode) {
  case Xtensa::G_LOAD:
  case Xtensa::G_ZEXTLOAD:
    return getLoadOpcode(ByteSize);
  case Xtensa::G_SEXTLOAD:
    return getSextLoadOpcode(ByteSize);
  case Xtensa::G_STORE:
    return getStoreOpcode(ByteSize);
  }

  return None;
}

struct ICmpInfo {
  CmpInst::Predicate Pred;
  unsigned Opcode;
  bool InvertCmp;
  bool InvertSelect;
};

static Optional<ICmpInfo> getICmpZeroConversionInfo(CmpInst::Predicate Pred) {
  ICmpInfo Info[] = {
      {CmpInst::ICMP_EQ, Xtensa::MOVEQZ, false, false},
      {CmpInst::ICMP_NE, Xtensa::MOVEQZ, false, true},
      {CmpInst::ICMP_SGT, Xtensa::MOVGEZ, true, true},
      {CmpInst::ICMP_SGE, Xtensa::MOVGEZ, false, false},
      {CmpInst::ICMP_SLT, Xtensa::MOVGEZ, false, true},
      {CmpInst::ICMP_SLE, Xtensa::MOVGEZ, true, false},
  };

  auto *It = std::find_if(std::begin(Info), std::end(Info),
                          [&](const auto &Info) { return Info.Pred == Pred; });
  if (It == std::end(Info)) {
    return None;
  }

  return *It;
}

bool XtensaInstructionSelector::select(MachineInstr &I) {
  if (I.getOpcode() == Xtensa::COPY) {
    return forceConstrainInstrRegisters(I);
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

void XtensaInstructionSelector::constrainInstrRegisters(
    MachineInstr &MI) const {
  constrainSelectedInstRegOperands(MI, TII, TRI, RBI);
}

bool XtensaInstructionSelector::forceConstrainInstrRegisters(MachineInstr &MI) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  for (MachineOperand &MO : MI.operands()) {
    if (!MO.isReg()) {
      continue;
    }

    Register Reg = MO.getReg();
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

bool XtensaInstructionSelector::emitCopy(MachineInstr &I, Register Dest,
                                         Register Src) {
  MachineInstr *CopyInst =
      emitInstrFor(I, Xtensa::COPY).addDef(Dest).addReg(Src);
  return forceConstrainInstrRegisters(*CopyInst);
}

bool XtensaInstructionSelector::emitL32R(MachineInstr &I, Register Dest,
                                         const Constant *Value) {
  MachineInstr *L32 =
      TII.loadConstWithL32R(*I.getParent(), I, I.getDebugLoc(), Dest, Value);
  constrainInstrRegisters(*L32);
  return true;
}

void XtensaInstructionSelector::emitAddParts(MachineInstr &I, Register Dest,
                                             Register Operand,
                                             const AddConstParts &Parts) {
  Register AddmiDest = Dest;
  Register AddiSrc = Operand;

  if (!Parts.Middle && !Parts.Low) {
    // Make sure adding 0 still emits at least something.
    emitCopy(I, Dest, Operand);
    return;
  }

  if (Parts.Middle && Parts.Low) {
    // We need two opcodes, so create a new temporary register.
    Register TempReg = createVirtualGPR();
    AddmiDest = TempReg;
    AddiSrc = TempReg;
  }

  if (Parts.Middle) {
    MachineInstr *AddMi = emitInstrFor(I, Xtensa::ADDMI)
                              .addDef(AddmiDest)
                              .addReg(Operand)
                              .addImm(Parts.Middle);
    constrainInstrRegisters(*AddMi);
  }

  if (Parts.Low) {
    MachineInstr *Addi = emitInstrFor(I, Xtensa::ADDI)
                             .addDef(Dest)
                             .addReg(AddiSrc)
                             .addImm(Parts.Low);
    constrainInstrRegisters(*Addi);
  }
}

bool XtensaInstructionSelector::emitICmpSelect(MachineInstr &I,
                                               CmpInst::Predicate Pred,
                                               Register CmpLHS, Register CmpRHS,
                                               Register TrueVal,
                                               Register FalseVal) {
  auto MaybeInfo = getICmpZeroConversionInfo(Pred);
  if (!MaybeInfo) {
    return false;
  }
  ICmpInfo ZeroConversionInfo = *MaybeInfo;

  MachineRegisterInfo &MRI = MF->getRegInfo();

  if (ZeroConversionInfo.InvertCmp) {
    std::swap(CmpLHS, CmpRHS);
  }

  if (ZeroConversionInfo.InvertSelect) {
    std::swap(TrueVal, FalseVal);
  }

  Register Selector = CmpLHS;
  if (!mi_match(CmpRHS, MRI, m_SpecificICst(0))) {
    // Convert the comparison to a subtraction followed by a comparison with 0.
    Selector = createVirtualGPR();
    MachineIRBuilder Builder(I);
    if (!select(*Builder.buildSub(Selector, CmpLHS, CmpRHS))) {
      return false;
    }
  }

  MachineInstr *MovMI = emitInstrFor(I, ZeroConversionInfo.Opcode)
                            .add(I.getOperand(0))
                            .addReg(FalseVal)
                            .addReg(TrueVal)
                            .addReg(Selector);
  constrainInstrRegisters(*MovMI);
  return true;
}

bool XtensaInstructionSelector::preISelLower(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_PTR_ADD:
    return convertPtrAddToAdd(I);
  case Xtensa::G_LOAD:
  case Xtensa::G_ZEXTLOAD:
  case Xtensa::G_SEXTLOAD:
    convertPtrLoad(I);
    break;
  case Xtensa::G_STORE:
    convertPtrStore(I);
    break;
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

void XtensaInstructionSelector::convertPtrLoad(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  // Convert pointer loads to integer loads so that our generated patterns
  // match. Since uses are always selected before defs, we can safely change
  // the destination type without an intermediary copy.
  Register Dest = I.getOperand(0).getReg();
  if (MRI.getType(Dest).isPointer()) {
    MRI.setType(Dest, LLT::scalar(32));
  }
}

bool XtensaInstructionSelector::convertPtrStore(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  Register Value = I.getOperand(0).getReg();
  if (MRI.getType(Value).isPointer()) {
    // There may be other uses/defs of the value that need to be integers, so
    // emit a pointer-to-integer copy feeding into the store.
    Register IntReg = createVirtualGPR();
    if (!emitCopy(I, IntReg, Value)) {
      return false;
    }
    I.getOperand(0).setReg(IntReg);
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

InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectFrameIndexOff(MachineOperand &Root) const {
  const MachineRegisterInfo &MRI = MF->getRegInfo();

  MachineInstr *RootMI = MRI.getVRegDef(Root.getReg());

  int FrameIndex = 0;
  int64_t Offset = 0;
  if (RootMI->getOpcode() == Xtensa::G_FRAME_INDEX) {
    FrameIndex = RootMI->getOperand(1).getIndex();
  } else {
    Register Base;
    if (!mi_match(RootMI, MRI, m_GPtrAdd(m_Reg(Base), m_ICst(Offset)))) {
      return None;
    }

    MachineInstr *BaseMI = MRI.getVRegDef(Base);
    if (BaseMI->getOpcode() != Xtensa::G_FRAME_INDEX) {
      return None;
    }

    FrameIndex = BaseMI->getOperand(1).getIndex();
  }

  return {{
      [=](MachineInstrBuilder &MIB) { MIB.addFrameIndex(FrameIndex); },
      [=](MachineInstrBuilder &MIB) { MIB.addImm(Offset); },
  }};
}

void XtensaInstructionSelector::renderImmPlus1(MachineInstrBuilder &MIB,
                                               const MachineInstr &I,
                                               int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_CONSTANT && OpIdx == -1 &&
         "Expected G_CONSTANT");
  MIB.addImm(I.getOperand(1).getCImm()->getSExtValue() + 1);
}

void XtensaInstructionSelector::renderBittestMask(MachineInstrBuilder &MIB,
                                                  const MachineInstr &I,
                                                  int OpIdx) const {
  assert(I.getOpcode() == TargetOpcode::G_CONSTANT && OpIdx == -1 &&
         "Expected G_CONSTANT");
  MIB.addImm(countTrailingZeros(I.getOperand(1).getCImm()->getZExtValue()));
}

bool XtensaInstructionSelector::selectEarly(MachineInstr &I) {
  switch (I.getOpcode()) {
  case Xtensa::G_PHI:
    I.setDesc(TII.get(Xtensa::PHI));
    return forceConstrainInstrRegisters(I);
  case Xtensa::G_AND:
    return selectAndAsExtui(I);
  case Xtensa::G_SELECT:
    return selectSelectFedByICmp(I);
  case Xtensa::G_SEXT_INREG:
    return selectSextInreg(I);
  case Xtensa::G_ADD:
  case Xtensa::G_SUB:
    return selectAddSubConst(I);
  case Xtensa::G_PTRTOINT:
  case Xtensa::G_INTTOPTR:
    I.setDesc(TII.get(Xtensa::COPY));
    return forceConstrainInstrRegisters(I);
  case Xtensa::G_FRAME_INDEX:
    I.setDesc(TII.get(Xtensa::ADDI));
    I.addOperand(MachineOperand::CreateImm(0));
    constrainInstrRegisters(I);
    return true;
  case Xtensa::G_GLOBAL_VALUE:
    if (!emitL32R(I, I.getOperand(0).getReg(), I.getOperand(1).getGlobal())) {
      return false;
    }
    I.eraseFromParent();
    return true;
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
  constrainInstrRegisters(*Extui);
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

bool XtensaInstructionSelector::selectSelectFedByICmp(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();
  auto *ICmp = getOpcodeDef<GICmp>(I.getOperand(1).getReg(), MRI);
  if (!ICmp) {
    return false;
  }

  auto Pred =
      static_cast<CmpInst::Predicate>(ICmp->getOperand(1).getPredicate());
  if (!emitICmpSelect(I, Pred, ICmp->getLHSReg(), ICmp->getRHSReg(),
                      I.getOperand(2).getReg(), I.getOperand(3).getReg())) {
    return false;
  }

  I.removeFromParent();
  return true;
}

bool XtensaInstructionSelector::selectSextInreg(MachineInstr &I) {
  unsigned OrigWidth = I.getOperand(2).getImm();

  // Note: `sext` wants the position of the sign bit, not the original bit
  // width.
  if (OrigWidth >= 8 && OrigWidth <= 23) {
    I.setDesc(TII.get(Xtensa::SEXT));
    I.getOperand(2).setImm(OrigWidth - 1);
    constrainInstrRegisters(I);
    return true;
  }

  unsigned ShiftAmount = 32 - OrigWidth;
  Register TempReg = createVirtualGPR();

  MachineInstr *SLLI = emitInstrFor(I, Xtensa::SLLI)
                           .addDef(TempReg)
                           .add(I.getOperand(1))
                           .addImm(ShiftAmount);
  constrainInstrRegisters(*SLLI);

  MachineInstr *SRAI = emitInstrFor(I, Xtensa::SRAI)
                           .add(I.getOperand(0))
                           .addReg(TempReg)
                           .addImm(ShiftAmount);
  constrainInstrRegisters(*SRAI);

  I.eraseFromParent();
  return true;
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

  MachineInstr *OtherOperandMI = MRI.getVRegDef(OtherOperand);
  if (selectFrameIndexOffset(I, *OtherOperandMI, ConstValue)) {
    return true;
  }

  Optional<AddConstParts> Parts = splitAddConst(ConstValue);
  if (!Parts) {
    return false;
  }

  // Heuristic: this is a big constant, so if it is used elsewhere and we would
  // need two instructions anyway, it's probably better to just let the original
  // constant be materialized.
  if (Parts->Middle && Parts->Low && !MRI.hasOneNonDBGUse(ConstOperand)) {
    return false;
  }

  emitAddParts(I, Dest, OtherOperand, *Parts);
  I.removeFromParent();
  return true;
}

bool XtensaInstructionSelector::selectFrameIndexOffset(MachineInstr &I,
                                                       MachineInstr &OperandMI,
                                                       int64_t Offset) {
  if (OperandMI.getOpcode() != Xtensa::G_FRAME_INDEX) {
    return false;
  }

  MachineInstr *Addi = emitInstrFor(I, Xtensa::ADDI)
                           .add(I.getOperand(0))
                           .add(OperandMI.getOperand(1))
                           .addImm(Offset);
  constrainInstrRegisters(*Addi);

  I.eraseFromParent();
  return true;
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
  case Xtensa::G_ICMP:
    return selectICmp(I);
  case Xtensa::G_LOAD:
  case Xtensa::G_SEXTLOAD:
  case Xtensa::G_ZEXTLOAD:
  case Xtensa::G_STORE:
    return selectLoadStore(I);
  case Xtensa::G_XTENSA_SSR_MASKED:
  case Xtensa::G_XTENSA_SSR_INRANGE:
    // We mutate the opcode manually due to a tablegen bug with implicit defs:
    // the opcode mutation logic currently doesn't take any implicit defs in the
    // source instruction into account, and so always manually adds an extra
    // `SAR` def.
    I.setDesc(TII.get(Xtensa::SSR));
    constrainInstrRegisters(I);
    return true;
  case Xtensa::G_XTENSA_SSL_MASKED:
  case Xtensa::G_XTENSA_SSL_INRANGE:
    I.setDesc(TII.get(Xtensa::SSL));
    constrainInstrRegisters(I);
    return true;
  }

  return false;
}

bool XtensaInstructionSelector::selectICmp(MachineInstr &I) {
  auto Pred = static_cast<CmpInst::Predicate>(I.getOperand(1).getPredicate());
  if (Pred != CmpInst::ICMP_EQ && Pred != CmpInst::ICMP_NE) {
    // Should have been handled earlier.
    return false;
  }

  Register Zero = createVirtualGPR();
  Register One = createVirtualGPR();
  constrainInstrRegisters(
      *emitInstrFor(I, Xtensa::MOVI).addDef(Zero).addImm(0));
  constrainInstrRegisters(*emitInstrFor(I, Xtensa::MOVI).addDef(One).addImm(1));

  if (!emitICmpSelect(I, Pred, I.getOperand(2).getReg(),
                      I.getOperand(3).getReg(), One, Zero)) {
    return false;
  }

  I.eraseFromParent();
  return true;
}

bool XtensaInstructionSelector::selectLoadStore(MachineInstr &I) {
  MachineRegisterInfo &MRI = MF->getRegInfo();

  Register Base = I.getOperand(1).getReg();
  int64_t Offset = 0;
  mi_match(Base, MRI, m_GPtrAdd(m_Reg(Base), m_ICst(Offset)));

  if (!I.hasOneMemOperand()) {
    return false;
  }

  MachineMemOperand *MMO = I.memoperands()[0];
  uint64_t ByteSize = MMO->getSize();

  Optional<unsigned> MaybeOpcode = getLoadStoreOpcode(I.getOpcode(), ByteSize);
  if (!MaybeOpcode) {
    return false;
  }
  unsigned Opcode = *MaybeOpcode;

  uint16_t InlineOffset = 0;
  AddConstParts PreAddParts = {0, 0};
  if (auto OffParts = splitOffsetConst(Offset, Log2_32(ByteSize))) {
    InlineOffset = OffParts->Offset;
    PreAddParts.Low = OffParts->LowAdd;
    PreAddParts.Middle = OffParts->MiddleAdd;
  } else {
    // We couldn't fold the offset into the instruction, so just use the operand
    // provided to the instruction with a 0 offset and let large offset be
    // handled later.
    Base = I.getOperand(1).getReg();
  }

  Register PreAdd = createVirtualGPR();
  emitAddParts(I, PreAdd, Base, PreAddParts);
  emitInstrFor(I, Opcode)
      .add(I.getOperand(0))
      .addReg(PreAdd)
      .addImm(InlineOffset)
      .addMemOperand(MMO);

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
