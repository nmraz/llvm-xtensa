#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-shift-lowering"

using namespace llvm;
using namespace MIPatternMatch;

namespace {

bool isLegalConstantShift(uint64_t Value) { return Value < 32; }

Optional<Register> matchRevShiftAmount(Register ShiftAmount,
                                       const MachineRegisterInfo &MRI) {
  Register RevShiftAmount;
  if (mi_match(ShiftAmount, MRI,
               m_GSub(m_SpecificICst(32), m_Reg(RevShiftAmount)))) {
    return RevShiftAmount;
  }

  return None;
}

Register buildRevShiftAmount(MachineInstr &MI, MachineRegisterInfo &MRI,
                             Register ShiftAmount) {
  LLT S32 = LLT::scalar(32);

  MachineIRBuilder B(MI);
  Register RevShiftAmount = MRI.createGenericVirtualRegister(S32);
  Register Const32 = MRI.createGenericVirtualRegister(S32);
  B.buildConstant(Const32, 32);
  B.buildSub(RevShiftAmount, Const32, ShiftAmount);
  return RevShiftAmount;
}

unsigned getXtensaShiftOpcode(unsigned GenericOpcode) {
  switch (GenericOpcode) {
  case Xtensa::G_SHL:
    return Xtensa::SLL;
  case Xtensa::G_LSHR:
    return Xtensa::SRL;
  case Xtensa::G_ASHR:
    return Xtensa::SRA;
  default:
    llvm_unreachable("Unknown shift opcode!");
  }
}

class XtensaShiftLowering : public MachineFunctionPass {
public:
  static char ID;

  XtensaShiftLowering();

  StringRef getPassName() const override { return "XtensaShiftLowering"; }

  bool lower(MachineInstr &MI);

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

} // namespace

XtensaShiftLowering::XtensaShiftLowering() : MachineFunctionPass(ID) {
  initializeXtensaShiftLoweringPass(*PassRegistry::getPassRegistry());
}

bool XtensaShiftLowering::lower(MachineInstr &MI) {
  unsigned Opcode = MI.getOpcode();
  if (Opcode != Xtensa::G_SHL && Opcode != Xtensa::G_LSHR &&
      Opcode != Xtensa::G_ASHR) {
    return false;
  }

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  const TargetSubtargetInfo &ST = MF.getSubtarget();
  const TargetInstrInfo &TII = *ST.getInstrInfo();
  const RegisterBankInfo &RBI = *ST.getRegBankInfo();
  const TargetRegisterInfo &TRI = *ST.getRegisterInfo();

  Register ShiftAmount = MI.getOperand(2).getReg();

  Optional<int64_t> ShiftConst = getIConstantVRegSExtVal(ShiftAmount, MRI);
  if (ShiftConst && isLegalConstantShift(*ShiftConst)) {
    // Can be selected directly later
    return false;
  }

  bool NeedsRevShift = Opcode == Xtensa::G_SHL;
  if (NeedsRevShift) {
    // Fold `32 - (32 - shift)` to just `shift` here instead of requiring
    // another pass to do it later on.
    if (auto ExistingRevShiftAmount = matchRevShiftAmount(ShiftAmount, MRI)) {
      ShiftAmount = *ExistingRevShiftAmount;
    } else {
      ShiftAmount = buildRevShiftAmount(MI, MRI, ShiftAmount);
    }
  }

  BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(Xtensa::G_XTENSA_SET_SAR))
      .addReg(ShiftAmount);

  MachineInstr *ShiftMI =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(getXtensaShiftOpcode(Opcode)))
          .add(MI.getOperand(0))
          .add(MI.getOperand(1));
  constrainSelectedInstRegOperands(*ShiftMI, TII, TRI, RBI);

  MI.removeFromParent();
  return true;
}

bool XtensaShiftLowering::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  for (auto &MBB : MF) {
    for (MachineInstr &MI : make_early_inc_range(MBB)) {
      Changed |= lower(MI);
    }
  }

  return Changed;
}

void XtensaShiftLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  getSelectionDAGFallbackAnalysisUsage(AU);
  MachineFunctionPass::getAnalysisUsage(AU);
}

char XtensaShiftLowering::ID = 0;
INITIALIZE_PASS_BEGIN(XtensaShiftLowering, DEBUG_TYPE,
                      "Lower Xtensa shifts before instruction selection", false,
                      false)
INITIALIZE_PASS_END(XtensaShiftLowering, DEBUG_TYPE,
                    "Lower Xtensa shifts before instruction selection", false,
                    false)

namespace llvm {
FunctionPass *createXtensaShiftLowering() { return new XtensaShiftLowering(); }
} // end namespace llvm
