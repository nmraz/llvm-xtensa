#include "XtensaLegalizerInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
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

XtensaLegalizerInfo::XtensaLegalizerInfo(const XtensaSubtarget &STI) {
  using namespace TargetOpcode;

  const LLT S1 = LLT::scalar(1);
  const LLT S8 = LLT::scalar(8);
  const LLT S16 = LLT::scalar(16);
  const LLT S32 = LLT::scalar(32);
  const LLT S64 = LLT::scalar(64);
  const LLT P0 = LLT::pointer(0, 32);

  getActionDefinitionsBuilder(G_PHI).legalFor({S32, P0}).clampScalar(0, S32,
                                                                     S32);

  getActionDefinitionsBuilder(G_CONSTANT)
      .legalFor({S32})
      .customFor({P0})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder({G_IMPLICIT_DEF, G_FREEZE})
      .legalFor({S32, P0})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(G_BRCOND).legalFor({S32}).clampScalar(0, S32,
                                                                    S32);

  getActionDefinitionsBuilder(G_BRJT).legalFor({{P0, S32}});

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

  getActionDefinitionsBuilder(G_ABS).legalFor({S32}).minScalar(0, S32).lower();

  if (STI.hasMinMax()) {
    getActionDefinitionsBuilder({G_SMIN, G_UMIN, G_SMAX, G_UMAX})
        .legalFor({S32})
        .minScalar(0, S32)
        .lower();
  } else {
    getActionDefinitionsBuilder({G_SMIN, G_UMIN, G_SMAX, G_UMAX}).lower();
  }

  // Note: we narrow the shift amount before dealing with the shifted value, as
  // that can result in substantially less code generated.
  getActionDefinitionsBuilder({G_SHL, G_LSHR, G_ASHR})
      .legalFor({{S32, S32}})
      .customFor({{S64, S32}})
      .clampScalar(1, S32, S32)
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S64);

  getActionDefinitionsBuilder({G_FSHL, G_FSHR}).legalFor({{S32, S32}}).lower();

  getActionDefinitionsBuilder(G_BSWAP)
      .customFor({S32})
      .maxScalar(0, S32)
      .lower();

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

  getActionDefinitionsBuilder({G_MEMSET, G_MEMCPY, G_MEMMOVE}).libcall();

  getActionDefinitionsBuilder(G_PTR_ADD)
      .legalFor({{P0, S32}})
      .clampScalar(1, S32, S32);

  getActionDefinitionsBuilder(G_INTTOPTR)
      .legalFor({{P0, S32}})
      .clampScalar(1, S32, S32);
  getActionDefinitionsBuilder(G_PTRTOINT)
      .legalFor({{S32, P0}})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder({G_FRAME_INDEX, G_GLOBAL_VALUE, G_JUMP_TABLE})
      .legalFor({P0});
  getActionDefinitionsBuilder(G_DYN_STACKALLOC).lower();

  // Ext/trunc instructions should all be folded together during
  // legalization, meaning they are never legal in the final output;
  // everything should be 32-bit.
  getActionDefinitionsBuilder({G_ZEXT, G_SEXT, G_ANYEXT})
      .maxScalar(0, S32)
      .unsupported();
  getActionDefinitionsBuilder(G_TRUNC).maxScalar(1, S32).unsupported();

  getLegacyLegalizerInfo().computeTables();

  verify(*STI.getInstrInfo());
}

bool XtensaLegalizerInfo::legalizeCustom(LegalizerHelper &Helper,
                                         MachineInstr &MI) const {
  MachineIRBuilder &MIRBuilder = Helper.MIRBuilder;
  MachineRegisterInfo &MRI = *MIRBuilder.getMRI();
  GISelChangeObserver &Observer = Helper.Observer;

  switch (MI.getOpcode()) {
  case TargetOpcode::G_ICMP:
    return legalizeICmp(MI, MRI, MIRBuilder, Observer);
  case TargetOpcode::G_SHL:
  case TargetOpcode::G_LSHR:
  case TargetOpcode::G_ASHR:
    return legalizeShift(MI, Helper, MRI, MIRBuilder, Observer);
  case TargetOpcode::G_BSWAP:
    return legalizeBswap(MI, MRI, MIRBuilder, Observer);
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

bool XtensaLegalizerInfo::legalizeICmp(MachineInstr &MI,
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

bool XtensaLegalizerInfo::legalizeShift(MachineInstr &MI,
                                        LegalizerHelper &Helper,
                                        MachineRegisterInfo &MRI,
                                        MachineIRBuilder &MIRBuilder,
                                        GISelChangeObserver &Observer) const {
  const LLT S32 = LLT::scalar(32);
  const LLT S64 = LLT::scalar(64);

  unsigned Opcode = MI.getOpcode();
  Register Dest = MI.getOperand(0).getReg();
  Register Input = MI.getOperand(1).getReg();
  Register ShiftAmount = MI.getOperand(2).getReg();

  LLT DestTy = MRI.getType(Dest);
  LLT ShiftAmountTy = MRI.getType(ShiftAmount);

  assert(DestTy == S64 && "Custom legalization attempted for non-64-bit shift");
  assert(
      ShiftAmountTy == S32 &&
      "Custom legalization attempted for shift with incorrectly-sized amount");

  if (auto VRegAndVal = getIConstantVRegValWithLookThrough(ShiftAmount, MRI)) {
    return Helper.narrowScalarShiftByConstant(
               MI, VRegAndVal->Value, S32, S32) == LegalizerHelper::Legalized;
  }

  // Adapted from `TargetLowering::expandShiftParts`

  bool IsShl = Opcode == TargetOpcode::G_SHL;
  bool IsAshr = Opcode == TargetOpcode::G_ASHR;

  Register InputHi = MRI.createGenericVirtualRegister(S32);
  Register InputLo = MRI.createGenericVirtualRegister(S32);
  MIRBuilder.buildUnmerge({InputLo, InputHi}, Input);

  auto LargeShiftCond = MIRBuilder.buildAnd(
      S32,
      MIRBuilder.buildLShr(S32, ShiftAmount, MIRBuilder.buildConstant(S32, 5)),
      MIRBuilder.buildConstant(S32, 1));

  auto MaskedShiftAmount =
      MIRBuilder.buildAnd(S32, ShiftAmount, MIRBuilder.buildConstant(S32, 31));

  auto LargeShiftFill =
      IsAshr ? MIRBuilder.buildAShr(S32, InputHi,
                                    MIRBuilder.buildConstant(S32, 31))
             : MIRBuilder.buildConstant(S32, 0);

  MachineInstrBuilder Funnel;
  MachineInstrBuilder MaskedShiftResult;

  if (IsShl) {
    Funnel = MIRBuilder.buildInstr(TargetOpcode::G_FSHL, {S32},
                                   {InputHi, InputLo, ShiftAmount});
    MaskedShiftResult = MIRBuilder.buildShl(S32, InputLo, MaskedShiftAmount);
  } else {
    Funnel = MIRBuilder.buildInstr(TargetOpcode::G_FSHR, {S32},
                                   {InputHi, InputLo, ShiftAmount});
    MaskedShiftResult =
        MIRBuilder.buildInstr(Opcode, {S32}, {InputHi, MaskedShiftAmount});
  }

  MachineInstrBuilder ResultHi;
  MachineInstrBuilder ResultLo;

  if (IsShl) {
    ResultHi =
        MIRBuilder.buildSelect(S32, LargeShiftCond, MaskedShiftResult, Funnel);
    ResultLo = MIRBuilder.buildSelect(S32, LargeShiftCond, LargeShiftFill,
                                      MaskedShiftResult);
  } else {
    ResultLo =
        MIRBuilder.buildSelect(S32, LargeShiftCond, MaskedShiftResult, Funnel);
    ResultHi = MIRBuilder.buildSelect(S32, LargeShiftCond, LargeShiftFill,
                                      MaskedShiftResult);
  }

  MIRBuilder.buildMerge(Dest, {ResultLo, ResultHi});
  MI.eraseFromParent();
  return true;
}

bool XtensaLegalizerInfo::legalizeBswap(MachineInstr &MI,
                                        MachineRegisterInfo &MRI,
                                        MachineIRBuilder &MIRBuilder,
                                        GISelChangeObserver &Observer) const {
  const LLT S32 = LLT::scalar(32);
  Register Dest = MI.getOperand(0).getReg();
  Register Input = MI.getOperand(1).getReg();

  assert(MRI.getType(Dest) == S32 &&
         "Custom lowering attempted for bad bswap width");

  // Convention: input bytes are numbered `1 2 3 4`, where 1 is the MSB.
  // We want to output `4 3 2 1` to `Dest`.

  auto Eight = MIRBuilder.buildConstant(S32, 8);
  auto Sixteen = MIRBuilder.buildConstant(S32, 16);

  // Build `X X 1 2`, where X is some arbitrary padding.
  auto Parts12 = MIRBuilder.buildLShr(S32, Input, Sixteen);

  // Build `2 1 2 3` by (funnel) shifting `X X 1 2 1 2 3 4` right by 8.
  auto Parts2123 = MIRBuilder.buildInstr(TargetOpcode::G_FSHR, {S32},
                                         {Parts12, Input, Eight});

  // Build `3 2 1 2` by rotating `2 1 2 3` right by 8.
  auto Parts3212 = MIRBuilder.buildInstr(TargetOpcode::G_FSHR, {S32},
                                         {Parts2123, Parts2123, Eight});

  // Build `4 3 2 1` by (funnel) shifting `1 2 3 4 3 2 1 2` right by 8.
  MIRBuilder.buildInstr(TargetOpcode::G_FSHR, {Dest},
                        {Input, Parts3212, Eight});

  MI.eraseFromParent();
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
