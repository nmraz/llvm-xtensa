#include "XtensaLegalizerInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/Support/LowLevelTypeImpl.h"
#include <cassert>

using namespace llvm;

static void convertPtrSrc(MachineIRBuilder &MIRBuilder, MachineInstr &MI,
                          unsigned OpIdx) {
  MachineOperand &Src = MI.getOperand(OpIdx);
  Register NewSrc =
      MIRBuilder.buildPtrToInt({LLT::scalar(32)}, {Src}).getReg(0);
  Src.setReg(NewSrc);
}

static void convertPtrDst(MachineRegisterInfo &MRI,
                          MachineIRBuilder &MIRBuilder, MachineInstr &MI,
                          unsigned OpIdx) {
  MachineOperand &Dst = MI.getOperand(OpIdx);
  Register NewDst = MRI.createGenericVirtualRegister(LLT::scalar(32));
  MIRBuilder.setInsertPt(MIRBuilder.getMBB(), ++MIRBuilder.getInsertPt());
  MIRBuilder.buildIntToPtr({Dst}, {NewDst});
  Dst.setReg(NewDst);
}

XtensaLegalizerInfo::XtensaLegalizerInfo(const XtensaSubtarget &ST) {
  using namespace TargetOpcode;

  const LLT S1 = LLT::scalar(1);
  const LLT S8 = LLT::scalar(8);
  const LLT S16 = LLT::scalar(16);
  const LLT S32 = LLT::scalar(32);
  const LLT P0 = LLT::pointer(0, 32);

  getActionDefinitionsBuilder(G_PHI).legalFor({S32, P0}).clampScalar(0, S32,
                                                                     S32);

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({S32})
      .customFor({P0})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(G_IMPLICIT_DEF)
      .legalFor({S32, P0})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(G_BRCOND).legalFor({S32}).clampScalar(0, S32,
                                                                    S32);

  getActionDefinitionsBuilder(G_SELECT)
      .legalFor({{S32, S32}})
      .customFor({{P0, S32}})
      .clampScalar(1, S32, S32)
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(
      {G_AND, G_OR, G_XOR, G_ADD, G_SUB, G_MUL, G_UMULH, G_SMULH})
      .legalFor({S32})
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(
      {G_SADDE, G_SSUBE, G_UADDE, G_USUBE, G_SADDO, G_SSUBO, G_UADDO, G_USUBO})
      .lowerFor({{S32, S1}})
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  // Note: we narrow the shift amount before dealing with the shifted value, as
  // that can result in substantially less code generated.
  getActionDefinitionsBuilder({G_SHL, G_LSHR, G_ASHR})
      .legalFor({{S32, S32}})
      .clampScalar(1, S32, S32)
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder({G_SDIV, G_UDIV, G_SREM, G_UREM})
      .legalFor({S32})
      .widenScalarToNextPow2(0)
      .minScalar(0, S32)
      .libcall();

  // TODO: create a custom libcall that does this in one operation for wide
  // values.
  getActionDefinitionsBuilder({G_SDIVREM, G_UDIVREM}).lower();

  getActionDefinitionsBuilder(G_ICMP)
      .legalFor({{S32, S32}})
      .customFor({{S32, P0}})
      .clampScalar(0, S32, S32)
      .clampScalar(1, S32, S32);

  getActionDefinitionsBuilder(G_SEXT_INREG).legalFor({S32}).lower();

  getActionDefinitionsBuilder({G_LOAD, G_ZEXTLOAD})
      .legalForTypesWithMemDesc({
          {S32, P0, S8, 8},
          {S32, P0, S16, 16},
          {S32, P0, S32, 32},
          {P0, P0, S32, 32},
      })
      .clampScalar(0, S32, S32)
      .lower();

  getActionDefinitionsBuilder(G_SEXTLOAD)
      .legalForTypesWithMemDesc({{S32, P0, S16, 16}})
      .clampScalar(0, S32, S32)
      .lower();

  getActionDefinitionsBuilder(G_STORE)
      .legalForTypesWithMemDesc({
          {S32, P0, S8, 8},
          {S32, P0, S16, 16},
          {S32, P0, S32, 32},
          {P0, P0, S32, 32},
      })
      .clampScalar(0, S32, S32)
      .lower();

  getActionDefinitionsBuilder(G_PTR_ADD)
      .legalFor({{P0, S32}})
      .clampScalar(1, S32, S32);

  getActionDefinitionsBuilder(G_INTTOPTR)
      .legalFor({{P0, S32}})
      .clampScalar(1, S32, S32);
  getActionDefinitionsBuilder(G_PTRTOINT)
      .legalFor({{S32, P0}})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder({G_FRAME_INDEX, G_GLOBAL_VALUE}).legalFor({P0});

  // Ext/trunc instructions should all be folded together during
  // legalization, meaning they are never legal in the final output;
  // everything should be 32-bit.
  getActionDefinitionsBuilder({G_ZEXT, G_SEXT, G_ANYEXT})
      .legalIf([](const LegalityQuery &Query) { return false; })
      .maxScalar(0, S32);

  getActionDefinitionsBuilder(G_TRUNC)
      .legalIf([](const LegalityQuery &Query) { return false; })
      .maxScalar(1, S32);

  getLegacyLegalizerInfo().computeTables();

  verify(*ST.getInstrInfo());
}

bool XtensaLegalizerInfo::legalizeCustom(LegalizerHelper &Helper,
                                         MachineInstr &MI) const {
  MachineIRBuilder &MIRBuilder = Helper.MIRBuilder;
  MachineRegisterInfo &MRI = *MIRBuilder.getMRI();
  GISelChangeObserver &Observer = Helper.Observer;

  switch (MI.getOpcode()) {
  case TargetOpcode::G_ICMP:
    return legalizeIcmp(MI, MRI, MIRBuilder, Observer);
  case TargetOpcode::G_CONSTANT:
    return legalizeConstant(MI, MRI, MIRBuilder, Observer);
  case TargetOpcode::G_SELECT:
    return legalizeSelect(MI, MRI, MIRBuilder, Observer);
  }
  return false;
}

bool XtensaLegalizerInfo::legalizeConstant(
    MachineInstr &MI, MachineRegisterInfo &MRI, MachineIRBuilder &MIRBuilder,
    GISelChangeObserver &Observer) const {
  LLT PtrTy = MRI.getType(MI.getOperand(0).getReg());
  assert(PtrTy.isPointer() &&
         "Custom legalization attempted for non-pointer constant");

  Observer.changedInstr(MI);
  convertPtrDst(MRI, MIRBuilder, MI, 0);
  Observer.changedInstr(MI);

  return true;
}

bool XtensaLegalizerInfo::legalizeIcmp(MachineInstr &MI,
                                       MachineRegisterInfo &MRI,
                                       MachineIRBuilder &MIRBuilder,
                                       GISelChangeObserver &Observer) const {
  LLT PtrTy = MRI.getType(MI.getOperand(2).getReg());
  assert(PtrTy.isPointer() &&
         "Custom legalization attempted for non-pointer icmp");

  Observer.changedInstr(MI);
  convertPtrSrc(MIRBuilder, MI, 2);
  convertPtrSrc(MIRBuilder, MI, 3);
  Observer.changedInstr(MI);

  return true;
}

bool XtensaLegalizerInfo::legalizeSelect(MachineInstr &MI,
                                         MachineRegisterInfo &MRI,
                                         MachineIRBuilder &MIRBuilder,
                                         GISelChangeObserver &Observer) const {
  LLT PtrTy = MRI.getType(MI.getOperand(0).getReg());
  assert(PtrTy.isPointer() &&
         "Custom legalization attempted for non-pointer select");

  Observer.changingInstr(MI);
  convertPtrSrc(MIRBuilder, MI, 2);
  convertPtrSrc(MIRBuilder, MI, 3);
  convertPtrDst(MRI, MIRBuilder, MI, 0);
  Observer.changedInstr(MI);

  return true;
}
