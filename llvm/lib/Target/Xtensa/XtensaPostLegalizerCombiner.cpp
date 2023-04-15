#include "MCTargetDesc/XtensaBaseInfo.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GISelKnownBits.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetMachine.h"
#include <cstdint>

#define DEBUG_TYPE "xtensa-postlegalizer-combiner"

using namespace llvm;
using namespace MIPatternMatch;

static bool matchICmpPow2Mask(const MachineRegisterInfo &MRI, MachineInstr &MI,
                              BuildFnTy &BuildFn) {
  Register TestValue;
  int64_t Mask;
  CmpInst::Predicate Pred;
  if (!mi_match(MI, MRI,
                m_GICmp(m_Pred(Pred), m_GAnd(m_Reg(TestValue), m_ICst(Mask)),
                        m_SpecificICst(0)))) {
    return false;
  }

  if (Pred != CmpInst::ICMP_EQ && Pred != CmpInst::ICMP_NE) {
    return false;
  }

  if (!isPowerOf2_32(Mask)) {
    return false;
  }

  Register Dest = MI.getOperand(0).getReg();
  uint32_t BitPos = countTrailingZeros(static_cast<uint32_t>(Mask));
  BuildFn = [=](MachineIRBuilder &Builder) {
    LLT S32 = LLT::scalar(32);

    auto One = Builder.buildConstant({S32}, 1);
    auto BitPosConst = Builder.buildConstant({S32}, BitPos);
    auto Shr = Builder.buildLShr({S32}, TestValue, BitPosConst);
    if (Pred == CmpInst::ICMP_NE) {
      Builder.buildAnd(Dest, Shr, One);
    } else {
      auto And = Builder.buildAnd({S32}, Shr, One);
      Builder.buildXor(Dest, And, One);
    }
  };

  return true;
}

static bool matchRepeatedXorConst(const CombinerHelper &Helper,
                                  const MachineRegisterInfo &MRI,
                                  MachineInstr &MI, Register &OrigReg) {
  int64_t C1 = 0;
  int64_t C2 = 0;

  Register Value;

  if (!mi_match(MI, MRI,
                m_GXor(m_GXor(m_Reg(Value), m_ICst(C1)), m_ICst(C2)))) {
    return false;
  }

  if (C1 != C2) {
    return false;
  }

  OrigReg = Value;
  return true;
}

static void applyRepeatedXorConst(CombinerHelper &Helper,
                                  MachineRegisterInfo &MRI, MachineInstr &MI,
                                  Register Value) {
  Helper.replaceRegWith(MRI, MI.getOperand(0).getReg(), Value);
  MI.eraseFromParent();
}

static bool isFoldableExpensiveICmp(const XtensaInstrInfo &TII,
                                    const MachineRegisterInfo &MRI,
                                    Register Reg) {
  if (!MRI.hasOneNonDBGUse(Reg)) {
    // If there are multiple uses, this is going to get messy anyway. Let's not
    // cause more trouble by increasing code size.
    return false;
  }

  GICmp *ICmp = getOpcodeDef<GICmp>(Reg, MRI);
  if (!ICmp) {
    return false;
  }

  return TII.intCmpRequiresSelect(ICmp->getCond());
}

static bool shouldCheckICmpRHS(unsigned Opcode) { return true; }
static bool shouldCheckICmpLHS(unsigned Opcode) {
  switch (Opcode) {
  case TargetOpcode::G_ADD:
  case TargetOpcode::G_AND:
  case TargetOpcode::G_MUL:
  case TargetOpcode::G_OR:
  case TargetOpcode::G_XOR:
    return true;
  default:
    return false;
  }
}

static bool matchExpensiveICmpOp(MachineRegisterInfo &MRI, MachineInstr &MI,
                                 BuildFnTy &BuildFn) {
  const XtensaSubtarget &STI =
      MI.getParent()->getParent()->getSubtarget<XtensaSubtarget>();
  const XtensaInstrInfo &TII = *STI.getInstrInfo();

  unsigned Opcode = MI.getOpcode();
  Register Dest = MI.getOperand(0).getReg();
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();

  Register ICmpReg;
  unsigned ICmpIdx = 0;
  if (shouldCheckICmpRHS(Opcode) && isFoldableExpensiveICmp(TII, MRI, RHS)) {
    ICmpReg = RHS;
    ICmpIdx = 1;
  } else if (shouldCheckICmpLHS(Opcode) &&
             isFoldableExpensiveICmp(TII, MRI, LHS)) {
    ICmpReg = LHS;
    ICmpIdx = 0;
  } else {
    return false;
  }

  LLVM_DEBUG(dbgs() << "Found expensive ICmp on use " << ICmpIdx << " of:");
  LLVM_DEBUG(MI.dump());
  LLVM_DEBUG(dbgs() << "\tICmp:");
  LLVM_DEBUG(MRI.getVRegDef(ICmpReg)->dump());

  BuildFn = [=](MachineIRBuilder &Builder) {
    LLT S32 = LLT::scalar(32);
    auto Zero = Builder.buildConstant({S32}, 0);
    auto One = Builder.buildConstant({S32}, 1);

    SrcOp ZeroOps[] = {LHS, RHS};
    ZeroOps[ICmpIdx] = Zero;
    auto ZeroVal = Builder.buildInstr(Opcode, {S32}, ZeroOps);
    LLVM_DEBUG(dbgs() << "\tBuilt zero val:");
    LLVM_DEBUG(ZeroVal->dump());

    SrcOp OneOps[] = {LHS, RHS};
    OneOps[ICmpIdx] = One;
    auto OneVal = Builder.buildInstr(Opcode, {S32}, OneOps);
    LLVM_DEBUG(dbgs() << "\tBuilt one val:");
    LLVM_DEBUG(OneVal->dump());

    LLVM_ATTRIBUTE_UNUSED auto Select =
        Builder.buildSelect(Dest, ICmpReg, OneVal, ZeroVal);
    LLVM_DEBUG(dbgs() << "\tBuilt select:");
    LLVM_DEBUG(Select->dump());
  };

  return true;
}

static bool getMulShifts(uint64_t MulAmount, unsigned &BaseShiftAmount,
                         unsigned &SmallShiftAmount) {
  for (unsigned I : {0, 1, 2, 3}) {
    uint64_t AdjustedMulAmount = MulAmount - (1 << I);
    if (isPowerOf2_64(AdjustedMulAmount)) {
      BaseShiftAmount = Log2_64(AdjustedMulAmount);
      SmallShiftAmount = I;
      return true;
    }
  }

  return false;
}

static bool matchMulConst(MachineRegisterInfo &MRI, MachineInstr &MI,
                          BuildFnTy &BuildFn) {
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();

  Register InputReg;
  Register AmountReg;
  int64_t MulAmount;

  if (mi_match(LHS, MRI, m_ICst(MulAmount))) {
    AmountReg = LHS;
    InputReg = RHS;
  } else if (mi_match(RHS, MRI, m_ICst(MulAmount))) {
    AmountReg = RHS;
    InputReg = LHS;
  } else {
    return false;
  }

  if (!MulAmount) {
    // Will be handled by other combines.
    return false;
  }

  Register DestReg = MI.getOperand(0).getReg();
  const MachineFunction &MF = *MI.getParent()->getParent();

  // This is measured in cycles/instructions, depending on whether we are
  // optimizing for speed or size.
  int Budget = 0;
  // Note: we want to convert the multiply even if we end up exactly even in
  // terms of latency/size, because the multiplier is typically not pipelined -
  // any `mull` instruction we remove can help scheduling later.
  if (MF.getFunction().hasMinSize()) {
    if (XtensaII::isMoviNImm7(MulAmount)) {
      // We can't compete with the 5 bytes required for a `movi.n` + `mull` in
      // more than one instruction, as none of the instructions we emit have
      // narrow variants.
      Budget = 1;
    } else if (MRI.hasOneNonDBGUse(AmountReg) && !isInt<12>(MulAmount)) {
      // This multiplication would currently require 10 bytes:
      // constant pool (4) + `l32r` (3) + `mull` (3)
      // That leaves room for 3 (wide) instructions.
      Budget = 3;
    }
  } else {
    // The multiply itself should cost 2 cycles.
    // If the amount is only used for the multiply, we also have the cycle
    // required to load the immediate.
    Budget = 2 + MRI.hasOneNonDBGUse(AmountReg);
  }

  int Cost = 0;
  uint64_t AbsMulAmount = MulAmount < 0 ? -MulAmount : MulAmount;

  if (isPowerOf2_64(AbsMulAmount)) {
    unsigned ShiftAmount = Log2_64(AbsMulAmount);
    if (ShiftAmount > 0) {
      // Need an explicit shift
      Cost++;
    }
    if (MulAmount < 0) {
      // Need an extra `neg`
      Cost++;
    }

    if (Cost > Budget) {
      return false;
    }

    BuildFn = [=](MachineIRBuilder &MIB) {
      LLT S32 = LLT::scalar(32);
      auto Shift =
          MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, ShiftAmount));
      if (MulAmount >= 0) {
        MIB.buildCopy(DestReg, Shift);
      } else {
        MIB.buildSub(DestReg, MIB.buildConstant(S32, 0), Shift);
      }
    };
    return true;
  }

  unsigned BaseShiftAmount = 0;
  unsigned SmallShiftAmount = 0;

  if (!getMulShifts(AbsMulAmount, BaseShiftAmount, SmallShiftAmount)) {
    return false;
  }

  // Everything from now on costs at least 1 instruction (the `add`).
  Cost = 1;
  if (BaseShiftAmount <= 3 && SmallShiftAmount == 0) {
    // Special case a small base shift with no small shift, which can allow us
    // to form an ordinary add or `addx` in the opposite direction.
  } else if (BaseShiftAmount) {
    // Need an extra `slli`.
    Cost++;
  }

  if (MulAmount < 0) {
    // Need an extra `neg`.
    Cost++;
  }

  if (Cost > Budget) {
    return false;
  }

  BuildFn = [=](MachineIRBuilder &MIB) {
    LLT S32 = LLT::scalar(32);

    auto BaseShift =
        MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, BaseShiftAmount));
    Register ExtraAddend =
        SmallShiftAmount
            ? MIB.buildShl(S32, InputReg,
                           MIB.buildConstant(S32, SmallShiftAmount))
                  .getReg(0)
            : InputReg;
    auto Add = MIB.buildAdd(S32, BaseShift, ExtraAddend);
    if (MulAmount < 0) {
      MIB.buildSub(DestReg, MIB.buildConstant(S32, 0), Add);
    } else {
      MIB.buildCopy(DestReg, Add);
    }
  };

  return true;
}

#define XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "XtensaGenPostLegalizeGICombiner.inc"
#undef XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H
#include "XtensaGenPostLegalizeGICombiner.inc"
#undef XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H

class XtensaPostLegalizerCombinerInfo final : public CombinerInfo {
  GISelKnownBits *KB;
  MachineDominatorTree *MDT;

public:
  XtensaGenPostLegalizerCombinerHelperRuleConfig GeneratedRuleCfg;

  XtensaPostLegalizerCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize,
                                  const LegalizerInfo *LI, GISelKnownBits *KB,
                                  MachineDominatorTree *MDT)
      : CombinerInfo(false, true, LI, EnableOpt, OptSize, MinSize), KB(KB),
        MDT(MDT) {
    if (!GeneratedRuleCfg.parseCommandLineOption())
      report_fatal_error("Invalid rule identifier");
  }

  bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
               MachineIRBuilder &B) const override;
};

bool XtensaPostLegalizerCombinerInfo::combine(GISelChangeObserver &Observer,
                                              MachineInstr &MI,
                                              MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, KB, MDT, LInfo);
  XtensaGenPostLegalizerCombinerHelper Generated(GeneratedRuleCfg);

  return Generated.tryCombineAll(Observer, MI, B, Helper);
}

#define XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP
#include "XtensaGenPostLegalizeGICombiner.inc"
#undef XTENSAPOSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP

// Pass boilerplate
// ================

class XtensaPostLegalizerCombiner : public MachineFunctionPass {
public:
  static char ID;

  XtensaPostLegalizerCombiner();

  StringRef getPassName() const override {
    return "Xtensa Post-Legalizer Combiner";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void XtensaPostLegalizerCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
  AU.addRequired<GISelKnownBitsAnalysis>();
  AU.addPreserved<GISelKnownBitsAnalysis>();
  AU.addRequired<MachineDominatorTree>();
  AU.addPreserved<MachineDominatorTree>();
  AU.addRequired<GISelCSEAnalysisWrapperPass>();
  AU.addPreserved<GISelCSEAnalysisWrapperPass>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

XtensaPostLegalizerCombiner::XtensaPostLegalizerCombiner()
    : MachineFunctionPass(ID) {
  initializeXtensaPostLegalizerCombinerPass(*PassRegistry::getPassRegistry());
}

bool XtensaPostLegalizerCombiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;

  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOpt::None && !skipFunction(F);

  const XtensaSubtarget &ST = MF.getSubtarget<XtensaSubtarget>();
  const LegalizerInfo *LI = ST.getLegalizerInfo();

  GISelKnownBits *KB = &getAnalysis<GISelKnownBitsAnalysis>().get(MF);
  MachineDominatorTree *MDT = &getAnalysis<MachineDominatorTree>();
  XtensaPostLegalizerCombinerInfo PCInfo(EnableOpt, F.hasOptSize(),
                                         F.hasMinSize(), LI, KB, MDT);

  GISelCSEAnalysisWrapper &Wrapper =
      getAnalysis<GISelCSEAnalysisWrapperPass>().getCSEWrapper();
  GISelCSEInfo *CSEInfo = &Wrapper.get(TPC->getCSEConfig());

  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, CSEInfo);
}

char XtensaPostLegalizerCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(XtensaPostLegalizerCombiner, DEBUG_TYPE,
                      "Combine Xtensa machine instrs after legalization", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GISelKnownBitsAnalysis)
INITIALIZE_PASS_END(XtensaPostLegalizerCombiner, DEBUG_TYPE,
                    "Combine Xtensa machine instrs after legalization", false,
                    false)

namespace llvm {
FunctionPass *createXtensaPostLegalizerCombiner() {
  return new XtensaPostLegalizerCombiner();
}
} // end namespace llvm
