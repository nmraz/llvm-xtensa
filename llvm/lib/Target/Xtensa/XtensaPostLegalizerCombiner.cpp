#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
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
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "xtensa-postlegalizer-combiner"

using namespace llvm;
using namespace MIPatternMatch;

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
