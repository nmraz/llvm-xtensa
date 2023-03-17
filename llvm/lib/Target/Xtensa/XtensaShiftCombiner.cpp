#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GISelKnownBits.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-shift-combiner"

using namespace llvm;
using namespace MIPatternMatch;

struct SetSarLoweringInfo {
  unsigned Opcode;
  Optional<Register> Operand;
};

static Optional<Register> getInvInrangeAmount(const MachineRegisterInfo &MRI,
                                              GISelKnownBits &KB,
                                              Register Amount) {
  Register InvAmount;
  if (!mi_match(Amount, MRI, m_GSub(m_SpecificICst(32), m_Reg(InvAmount)))) {
    return None;
  }

  // In general, `SSR x` and `SSL (32 - x)` (and vice versa) differ only when
  // the low 5 bits of `x` are 0: SSR favors 0, while SSL favors 32.

  // We have an SS{RL}_INRANGE of `(32 - InvAmount)`, so we can assume that
  // `InvAmount` is in the (unsigned) range [1, 32]. If we can prove that
  // bit 5 of `InvAmount` is 0, it must be in the unsigned range [1, 31],
  // in which case we can convert to a SS{LR}_INRANGE.

  // The generic nonzero check on the remaining 5 bits will be performed later.

  if (!KB.getKnownBits(InvAmount).Zero[5]) {
    return None;
  }

  return InvAmount;
}

static bool matchLowerSetSarInrange(const MachineRegisterInfo &MRI,
                                    GISelKnownBits &KB, MachineInstr &MI,
                                    SetSarLoweringInfo &Info) {
  unsigned Opcode = MI.getOpcode();
  assert(Opcode == Xtensa::G_XTENSA_SSR_INRANGE ||
         Opcode == Xtensa::G_XTENSA_SSL_INRANGE);

  bool IsLeft = Opcode == Xtensa::G_XTENSA_SSL_INRANGE;
  Register Amount = MI.getOperand(0).getReg();

  if (auto InvAmount = getInvInrangeAmount(MRI, KB, Amount)) {
    // We have a proven-safe inverted shift setup, fold into a single
    // instruction.
    Info = {IsLeft ? Xtensa::G_XTENSA_SSR_INRANGE
                   : Xtensa::G_XTENSA_SSL_INRANGE,
            InvAmount};
    return true;
  }

  // We failed to match the special pattern, so use a standard masked shift
  // amount instead.
  Info = {IsLeft ? Xtensa::G_XTENSA_SSL_MASKED : Xtensa::G_XTENSA_SSR_MASKED,
          None};
  return true;
}

static bool matchSetSarMaskedRedundantAnd(const MachineRegisterInfo &MRI,
                                          GISelKnownBits &KB, MachineInstr &MI,
                                          Register &MatchedOperand) {
  Register Operand = MI.getOperand(0).getReg();

  Register MaskLHS;
  Register MaskRHS;
  if (!mi_match(Operand, MRI, m_GAnd(m_Reg(MaskLHS), m_Reg(MaskRHS)))) {
    return false;
  }

  // Anything that has bits 0-4 set is redundant
  APInt MinRedundantMask = APInt::getLowBitsSet(32, 5);
  auto IsRedundantMask = [&](Register Val) {
    return MinRedundantMask.isSubsetOf(KB.getKnownBits(Val).One);
  };

  if (IsRedundantMask(MaskLHS)) {
    MatchedOperand = MaskRHS;
    return true;
  }

  if (IsRedundantMask(MaskRHS)) {
    MatchedOperand = MaskLHS;
    return true;
  }

  return false;
}

static bool matchInvSetSarMasked(const MachineRegisterInfo &MRI,
                                 GISelKnownBits &KB, MachineInstr &MI,
                                 SetSarLoweringInfo &Info) {
  unsigned Opcode = MI.getOpcode();
  assert(Opcode == Xtensa::G_XTENSA_SSR_MASKED ||
         Opcode == Xtensa::G_XTENSA_SSL_MASKED);

  bool IsLeft = Opcode == Xtensa::G_XTENSA_SSL_MASKED;
  Register Amount = MI.getOperand(0).getReg();

  Register InvAmount;
  if (!mi_match(Amount, MRI, m_GSub(m_SpecificICst(32), m_Reg(InvAmount)))) {
    return false;
  }

  // As explained above, `SSR x` and `SSL (32 - x)` (and vice versa) differ only
  // when the low 5 bits of `x` are 0. If any of the low 5 bits can be proven
  // nonzero, the subtraction can safely be folded.
  if ((KB.getKnownBits(InvAmount).One & APInt::getLowBitsSet(32, 5)).isZero()) {
    return false;
  }

  Info = {IsLeft ? Xtensa::G_XTENSA_SSR_MASKED : Xtensa::G_XTENSA_SSL_MASKED,
          InvAmount};
  return true;
}

static void applySetSarLowering(const CombinerHelper &Helper,
                                MachineRegisterInfo &MRI, MachineInstr &MI,
                                const SetSarLoweringInfo &Info) {
  Helper.replaceOpcodeWith(MI, Info.Opcode);
  if (Info.Operand) {
    Helper.replaceRegOpWith(MRI, MI.getOperand(0), *Info.Operand);
  }
}

static uint64_t getMaskedSarValue(uint64_t Value) { return Value & 31; }

static bool matchSetSarMaskedConstAddSub(const CombinerHelper &Helper,
                                         MachineRegisterInfo &MRI,
                                         MachineInstr &MI, unsigned ConstOp,
                                         BuildFnTy &BuildFn) {
  assert(ConstOp == 1 || ConstOp == 2);

  MachineInstr *Amount = nullptr;
  if (!mi_match(MI.getOperand(0).getReg(), MRI,
                m_OneNonDBGUse(m_MInstr(Amount)))) {
    return false;
  }

  unsigned Opcode = Amount->getOpcode();
  if (Opcode != Xtensa::G_ADD && Opcode != Xtensa::G_SUB) {
    return false;
  }

  Optional<int64_t> MaybeConstVal =
      getIConstantVRegSExtVal(Amount->getOperand(ConstOp).getReg(), MRI);
  if (!MaybeConstVal) {
    return false;
  }

  uint64_t ConstVal = *MaybeConstVal;
  uint64_t MaskedConstVal = getMaskedSarValue(ConstVal);
  if (MaskedConstVal == ConstVal) {
    return false;
  }

  BuildFn = [=, &Helper, &MI, &MRI](MachineIRBuilder &B) {
    LLT S32 = LLT::scalar(32);
    SrcOp Srcs[] = {Amount->getOperand(1), Amount->getOperand(2)};

    B.setDebugLoc(Amount->getDebugLoc());
    auto MaskedConst = B.buildConstant(S32, MaskedConstVal);
    Srcs[ConstOp - 1] = MaskedConst;
    auto NewAmount = B.buildInstr(Opcode, {S32}, Srcs);
    Helper.replaceRegOpWith(MRI, MI.getOperand(0), NewAmount.getReg(0));
  };

  return true;
}

static bool matchUnmaskedConst(const MachineRegisterInfo &MRI, MachineInstr &MI,
                               int32_t &MatchInfo) {
  Optional<int64_t> MaybeConstVal =
      getIConstantVRegSExtVal(MI.getOperand(0).getReg(), MRI);
  if (!MaybeConstVal) {
    return false;
  }

  int64_t ConstVal = *MaybeConstVal;
  uint32_t MaskedConstVal = getMaskedSarValue(ConstVal);
  if (MaskedConstVal == ConstVal) {
    return false;
  }

  MatchInfo = MaskedConstVal;
  return true;
}

static void replaceOperandWithConstant(const CombinerHelper &Helper,
                                       MachineRegisterInfo &MRI,
                                       MachineIRBuilder &B, MachineOperand &Op,
                                       int32_t Value) {
  B.setInstrAndDebugLoc(*Op.getParent());
  Helper.replaceRegOpWith(
      MRI, Op, B.buildConstant(LLT::scalar(32), Value)->getOperand(0).getReg());
}

static bool matchSSLMaskedConst(const MachineRegisterInfo &MRI,
                                MachineInstr &MI, int32_t &MatchInfo) {
  assert(MI.getOpcode() == Xtensa::G_XTENSA_SSL_MASKED);
  if (!matchUnmaskedConst(MRI, MI, MatchInfo)) {
    return false;
  }

  // We can't replace `SSL 0`, as that actually needs to set SAR to 32.
  return MatchInfo != 0;
}

static void applySSLMaskedConst(const CombinerHelper &Helper,
                                MachineRegisterInfo &MRI, MachineIRBuilder &B,
                                MachineInstr &MI, int32_t MaskedValue) {
  assert(MaskedValue != 0 && "Attempting to invert SSL 0");
  Helper.replaceOpcodeWith(MI, Xtensa::G_XTENSA_SSR_MASKED);
  replaceOperandWithConstant(Helper, MRI, B, MI.getOperand(0),
                             32 - MaskedValue);
}

#define XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "XtensaGenShiftGICombiner.inc"
#undef XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_H
#include "XtensaGenShiftGICombiner.inc"
#undef XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_H

class XtensaShiftCombinerInfo final : public CombinerInfo {
  GISelKnownBits *KB;

public:
  XtensaGenShiftCombinerHelperRuleConfig GeneratedRuleCfg;

  XtensaShiftCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize,
                          GISelKnownBits *KB)
      : CombinerInfo(false, false, nullptr, EnableOpt, OptSize, MinSize),
        KB(KB) {
    if (!GeneratedRuleCfg.parseCommandLineOption())
      report_fatal_error("Invalid rule identifier");
  }

  bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
               MachineIRBuilder &B) const override;
};

bool XtensaShiftCombinerInfo::combine(GISelChangeObserver &Observer,
                                      MachineInstr &MI,
                                      MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, KB, nullptr, LInfo);
  XtensaGenShiftCombinerHelper Generated(GeneratedRuleCfg);

  return Generated.tryCombineAll(Observer, MI, B, Helper);
}

#define XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_CPP
#include "XtensaGenShiftGICombiner.inc"
#undef XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_CPP

// Pass boilerplate
// ================

class XtensaShiftCombiner : public MachineFunctionPass {
public:
  static char ID;

  XtensaShiftCombiner();

  StringRef getPassName() const override { return "Xtensa Shift Combiner"; }

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void XtensaShiftCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
  AU.addRequired<GISelKnownBitsAnalysis>();
  AU.addPreserved<GISelKnownBitsAnalysis>();
  AU.addRequired<GISelCSEAnalysisWrapperPass>();
  AU.addPreserved<GISelCSEAnalysisWrapperPass>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

XtensaShiftCombiner::XtensaShiftCombiner() : MachineFunctionPass(ID) {
  initializeXtensaShiftCombinerPass(*PassRegistry::getPassRegistry());
}

bool XtensaShiftCombiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;

  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOpt::None && !skipFunction(F);

  GISelKnownBits *KB = &getAnalysis<GISelKnownBitsAnalysis>().get(MF);
  XtensaShiftCombinerInfo PCInfo(EnableOpt, F.hasOptSize(), F.hasMinSize(), KB);

  GISelCSEAnalysisWrapper &Wrapper =
      getAnalysis<GISelCSEAnalysisWrapperPass>().getCSEWrapper();
  GISelCSEInfo *CSEInfo = &Wrapper.get(TPC->getCSEConfig());

  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, CSEInfo);
}

char XtensaShiftCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(XtensaShiftCombiner, DEBUG_TYPE,
                      "Combine Xtensa shift patterns", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GISelKnownBitsAnalysis)
INITIALIZE_PASS_END(XtensaShiftCombiner, DEBUG_TYPE,
                    "Combine Xtensa shift patterns", false, false)

namespace llvm {
FunctionPass *createXtensaShiftCombiner() { return new XtensaShiftCombiner(); }
} // end namespace llvm
