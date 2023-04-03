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

#define DEBUG_TYPE "xtensa-prelegalizer-combiner"

using namespace llvm;

namespace {
class XtensaPreLegalizerCombinerInfo final : public CombinerInfo {

public:
  XtensaPreLegalizerCombinerInfo(bool EnableOpt, bool OptSize, bool MinSize)
      : CombinerInfo(true, false, nullptr, EnableOpt, OptSize, MinSize) {}

  bool combine(GISelChangeObserver &Observer, MachineInstr &MI,
               MachineIRBuilder &B) const override;
};

bool XtensaPreLegalizerCombinerInfo::combine(GISelChangeObserver &Observer,
                                             MachineInstr &MI,
                                             MachineIRBuilder &B) const {
  CombinerHelper Helper(Observer, B);
  switch (MI.getOpcode()) {
  case TargetOpcode::G_MEMCPY:
  case TargetOpcode::G_MEMMOVE:
  case TargetOpcode::G_MEMSET:
    return Helper.tryCombineMemCpyFamily(MI);
  }
  return false;
}

// Pass boilerplate
// ================

class XtensaPreLegalizerCombiner : public MachineFunctionPass {
public:
  static char ID;

  XtensaPreLegalizerCombiner();

  StringRef getPassName() const override {
    return "Xtensa Pre-Legalizer Combiner";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
} // end anonymous namespace

void XtensaPreLegalizerCombiner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesCFG();
  MachineFunctionPass::getAnalysisUsage(AU);
}

XtensaPreLegalizerCombiner::XtensaPreLegalizerCombiner()
    : MachineFunctionPass(ID) {
  initializeXtensaPreLegalizerCombinerPass(*PassRegistry::getPassRegistry());
}

bool XtensaPreLegalizerCombiner::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;

  auto *TPC = &getAnalysis<TargetPassConfig>();
  const Function &F = MF.getFunction();
  bool EnableOpt =
      MF.getTarget().getOptLevel() != CodeGenOpt::None && !skipFunction(F);

  XtensaPreLegalizerCombinerInfo PCInfo(EnableOpt, F.hasOptSize(),
                                        F.hasMinSize());
  Combiner C(PCInfo, TPC);
  return C.combineMachineInstrs(MF, nullptr);
}

char XtensaPreLegalizerCombiner::ID = 0;
INITIALIZE_PASS_BEGIN(XtensaPreLegalizerCombiner, DEBUG_TYPE,
                      "Combine Xtensa machine instrs before legalization",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(XtensaPreLegalizerCombiner, DEBUG_TYPE,
                    "Combine Xtensa machine instrs before legalization", false,
                    false)

namespace llvm {
FunctionPass *createXtensaPreLegalizerCombiner() {
  return new XtensaPreLegalizerCombiner();
}
} // end namespace llvm
