#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GISelKnownBits.h"
#include "llvm/CodeGen/GlobalISel/GenericMachineInstrs.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "xtensa-O0-postlegalizer-combiner"

using namespace llvm;
using namespace MIPatternMatch;

static bool matchICmpAndOne(const MachineRegisterInfo &MRI, MachineInstr &MI,
                            Register &ICmpResult) {
  Register Operand;
  if (!mi_match(MI, MRI, m_GAnd(m_Reg(Operand), m_SpecificICst(1)))) {
    return false;
  }
  if (!getOpcodeDef<GICmp>(Operand, MRI)) {
    return false;
  }

  ICmpResult = Operand;
  return true;
}

static void applyICmpAndOne(CombinerHelper &Helper, MachineRegisterInfo &MRI,
                            MachineInstr &MI, Register ICmpResult) {
  Helper.replaceRegWith(MRI, MI.getOperand(0).getReg(), ICmpResult);
  MI.eraseFromParent();
}

#define XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "XtensaGenO0PostLegalizeGICombiner.inc"
#undef XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H
#include "XtensaGenO0PostLegalizeGICombiner.inc"
#undef XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_H

class XtensaO0PostLegalizerCombinerInfo final : public CombinerInfo {
public:
  XtensaGenO0PostLegalizerCombinerHelperRuleConfig GeneratedRuleCfg;

  XtensaO0PostLegalizerCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize,
                                    const LegalizerInfo *LI)
      : CombinerInfo(false, true, LI, EnableOpt, OptSize, MinSize) {
    if (!GeneratedRuleCfg.parseCommandLineOption())
      report_fatal_error("Invalid rule identifier");
  }

  bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
               MachineIRBuilder &B) const override;
};

bool XtensaO0PostLegalizerCombinerInfo::combine(GISelChangeObserver &Observer,
                                                MachineInstr &MI,
                                                MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, nullptr, nullptr, LInfo);
  XtensaGenO0PostLegalizerCombinerHelper Generated(GeneratedRuleCfg);

  return Generated.tryCombineAll(Observer, MI, B, Helper);
}

#define XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP
#include "XtensaGenO0PostLegalizeGICombiner.inc"
#undef XTENSAO0POSTLEGALIZERCOMBINERHELPER_GENCOMBINERHELPER_CPP

// Pass boilerplate
// ================

class XtensaO0PostLegalizerCombiner : public MachineFunctionPass {
public:
  static char ID;

  XtensaO0PostLegalizerCombiner();

  StringRef getPassName() const override {
    return "Xtensa O0 Post-Legalizer Combiner";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void XtensaO0PostLegalizerCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
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

XtensaO0PostLegalizerCombiner::XtensaO0PostLegalizerCombiner()
    : MachineFunctionPass(ID) {
  initializeXtensaO0PostLegalizerCombinerPass(*PassRegistry::getPassRegistry());
}

bool XtensaO0PostLegalizerCombiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;

  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();

  const XtensaSubtarget &ST = MF.getSubtarget<XtensaSubtarget>();
  const LegalizerInfo *LI = ST.getLegalizerInfo();

  XtensaO0PostLegalizerCombinerInfo PCInfo(false, F.hasOptSize(),
                                           F.hasMinSize(), LI);
  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, nullptr);
}

char XtensaO0PostLegalizerCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(XtensaO0PostLegalizerCombiner, DEBUG_TYPE,
                      "Combine Xtensa machine instrs after legalization", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(XtensaO0PostLegalizerCombiner, DEBUG_TYPE,
                    "Combine Xtensa machine instrs after legalization", false,
                    false)

namespace llvm {
FunctionPass *createXtensaO0PostLegalizerCombiner() {
  return new XtensaO0PostLegalizerCombiner();
}
} // end namespace llvm
