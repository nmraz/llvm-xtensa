#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/GlobalISel/CSEInfo.h"
#include "llvm/CodeGen/GlobalISel/Combiner.h"
#include "llvm/CodeGen/GlobalISel/CombinerHelper.h"
#include "llvm/CodeGen/GlobalISel/CombinerInfo.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>

#define DEBUG_TYPE "xtensa-shift-combiner"

using namespace llvm;
using namespace MIPatternMatch;

namespace {

bool matchLowerSetSar31(const MachineRegisterInfo &MRI, MachineInstr &MI,
                        unsigned &NewOpcode) {
  assert(MI.getOpcode() == Xtensa::G_XTENSA_SET_SAR31);

  Register Operand = MI.getOperand(0).getReg();
  if (mi_match(Operand, MRI, m_GSub(m_SpecificICst(32), m_Reg()))) {
    // Relax to a `SET_SAR32` if it may help the instruction selector match to
    // a left-shift-setting opcode.
    NewOpcode = Xtensa::G_XTENSA_SET_SAR32;
    return true;
  }

  NewOpcode = Xtensa::G_XTENSA_SET_SAR_MASKED;
  return true;
}

bool matchSetSarMaskedRedundantMask(const MachineRegisterInfo &MRI,
                                    MachineInstr &MI, Register &MaskedOperand) {
  assert(MI.getOpcode() == Xtensa::G_XTENSA_SET_SAR_MASKED);
  Register Operand = MI.getOperand(0).getReg();
  return mi_match(Operand, MRI,
                  m_GAnd(m_SpecificICst(0x1f), m_Reg(MaskedOperand)));
}

} // namespace

#define XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_DEPS
#include "XtensaGenShiftGICombiner.inc"
#undef XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_DEPS

namespace {
#define XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_H
#include "XtensaGenShiftGICombiner.inc"
#undef XTENSASHIFTCOMBINERHELPER_GENCOMBINERHELPER_H

class XtensaShiftCombinerInfo final : public CombinerInfo {
public:
  XtensaGenShiftCombinerHelperRuleConfig GeneratedRuleCfg;

  XtensaShiftCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize)
      : CombinerInfo(false, false, nullptr, EnableOpt, OptSize, MinSize) {
    if (!GeneratedRuleCfg.parseCommandLineOption())
      report_fatal_error("Invalid rule identifier");
  }

  bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
               MachineIRBuilder &B) const override;
};

bool XtensaShiftCombinerInfo::combine(GISelChangeObserver &Observer,
                                      MachineInstr &MI,
                                      MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B, nullptr, nullptr, LInfo);
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

  StringRef getPassName() const override { return "XtensaShiftCombiner"; }

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void XtensaShiftCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
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

  XtensaShiftCombinerInfo PCInfo(EnableOpt, F.hasOptSize(), F.hasMinSize());

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
INITIALIZE_PASS_END(XtensaShiftCombiner, DEBUG_TYPE,
                    "Combine Xtensa shift patterns", false, false)

namespace llvm {
FunctionPass *createXtensaShiftCombiner() { return new XtensaShiftCombiner(); }
} // end namespace llvm
