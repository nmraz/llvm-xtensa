#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/GISelKnownBits.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "xtensa-postlegalizer-combiner"

using namespace llvm;
using namespace MIPatternMatch;

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
