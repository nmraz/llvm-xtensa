#include "XtensaLegalizerInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/Support/LowLevelTypeImpl.h"
#include <cassert>

using namespace llvm;

XtensaLegalizerInfo::XtensaLegalizerInfo(const XtensaSubtarget &ST) {
  using namespace TargetOpcode;

  const LLT S1 = LLT::scalar(1);
  const LLT S8 = LLT::scalar(8);
  const LLT S16 = LLT::scalar(16);
  const LLT S32 = LLT::scalar(32);
  const LLT P0 = LLT::pointer(0, 32);

  getActionDefinitionsBuilder(G_PHI).legalFor({S32, P0}).clampScalar(0, S32,
                                                                     S32);

  getActionDefinitionsBuilder({G_CONSTANT, G_IMPLICIT_DEF})
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

  // TODO: there is a `sext` instruction in the miscellaneous instruction option
  // that does exactly this.
  getActionDefinitionsBuilder(G_SEXT_INREG).lower();

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
  case TargetOpcode::G_SELECT:
    return legalizeSelect(MI, MRI, MIRBuilder, Observer);
  }
  return false;
}

bool XtensaLegalizerInfo::legalizeIcmp(MachineInstr &MI,
                                       MachineRegisterInfo &MRI,
                                       MachineIRBuilder &MIRBuilder,
                                       GISelChangeObserver &Observer) const {
  Register LHS = MI.getOperand(2).getReg();
  Register RHS = MI.getOperand(3).getReg();

  LLT PtrTy = MRI.getType(LHS);
  assert(PtrTy.isPointer() &&
         "Custom legalization attempted for non-pointer icmp");
  LLT IntPtrTy = LLT::scalar(PtrTy.getSizeInBits());

  Register IntLHS = MIRBuilder.buildPtrToInt({IntPtrTy}, {LHS}).getReg(0);
  Register IntRHS = MIRBuilder.buildPtrToInt({IntPtrTy}, {RHS}).getReg(0);

  Observer.changedInstr(MI);
  MI.getOperand(2).ChangeToRegister(IntLHS, false);
  MI.getOperand(3).ChangeToRegister(IntRHS, false);
  Observer.changedInstr(MI);

  return true;
}

bool XtensaLegalizerInfo::legalizeSelect(MachineInstr &MI,
                                         MachineRegisterInfo &MRI,
                                         MachineIRBuilder &MIRBuilder,
                                         GISelChangeObserver &Observer) const {
  Register DstReg = MI.getOperand(0).getReg();
  Register TrueReg = MI.getOperand(2).getReg();
  Register FalseReg = MI.getOperand(3).getReg();

  LLT PtrTy = MRI.getType(DstReg);
  assert(PtrTy.isPointer() &&
         "Custom legalization attempted for non-pointer select");
  LLT IntPtrTy = LLT::scalar(PtrTy.getSizeInBits());

  Register DstIntReg = MRI.createGenericVirtualRegister(IntPtrTy);

  Register TrueIntReg =
      MIRBuilder.buildPtrToInt({IntPtrTy}, {TrueReg}).getReg(0);
  Register FalseIntReg =
      MIRBuilder.buildPtrToInt({IntPtrTy}, {FalseReg}).getReg(0);

  MIRBuilder.setInsertPt(MIRBuilder.getMBB(), ++MIRBuilder.getInsertPt());
  MIRBuilder.buildIntToPtr({DstReg}, {DstIntReg});

  Observer.changingInstr(MI);
  MI.getOperand(0).setReg(DstIntReg);
  MI.getOperand(2).setReg(TrueIntReg);
  MI.getOperand(3).setReg(FalseIntReg);
  Observer.changedInstr(MI);

  return true;
}
