#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/GlobalISel/MIPatternMatch.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-shift-lowering"

using namespace llvm;
using namespace MIPatternMatch;

class XtensaShiftLowering : public MachineFunctionPass {
public:
  static char ID;

  const MachineRegisterInfo *MRI;
  const TargetInstrInfo *TII;
  const TargetRegisterInfo *TRI;
  const RegisterBankInfo *RBI;

  XtensaShiftLowering();

  StringRef getPassName() const override { return "Xtensa Shift Lowering"; }

  bool lower(MachineInstr &MI);
  bool lowerStandardShift(MachineInstr &MI);
  bool lowerFunnelShift(MachineInstr &MI);

  bool runOnMachineFunction(MachineFunction &MF) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

XtensaShiftLowering::XtensaShiftLowering() : MachineFunctionPass(ID) {
  initializeXtensaShiftLoweringPass(*PassRegistry::getPassRegistry());
}

static bool isLegalConstantStandardShift(unsigned Opcode, uint64_t Value) {
  switch (Opcode) {
  case Xtensa::G_SHL:
    // SLLI doesn't support 0 immediates
    return Value > 0 && Value < 32;
  default:
    return Value < 32;
  }
}

static unsigned getStandardShiftOpcode(unsigned GenericOpcode) {
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

bool XtensaShiftLowering::lower(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case Xtensa::G_SHL:
  case Xtensa::G_LSHR:
  case Xtensa::G_ASHR:
    return lowerStandardShift(MI);
  case Xtensa::G_FSHL:
  case Xtensa::G_FSHR:
    return lowerFunnelShift(MI);
  default:
    return false;
  }
}

bool XtensaShiftLowering::lowerStandardShift(MachineInstr &MI) {
  unsigned Opcode = MI.getOpcode();
  MachineBasicBlock &MBB = *MI.getParent();
  Register ShiftAmount = MI.getOperand(2).getReg();

  Optional<int64_t> ShiftConst = getIConstantVRegSExtVal(ShiftAmount, *MRI);
  if (ShiftConst && isLegalConstantStandardShift(Opcode, *ShiftConst)) {
    // Can be selected directly later
    return false;
  }

  bool IsLeftShift = Opcode == Xtensa::G_SHL;

  // We can use the inrange instructions here, since LLVM defines shifts to
  // produce poison when the shift amount is greater than or equal to the bit
  // width.
  BuildMI(MBB, MI, MI.getDebugLoc(),
          TII->get(IsLeftShift ? Xtensa::G_XTENSA_SSL_INRANGE
                               : Xtensa::G_XTENSA_SSR_INRANGE))
      .addReg(ShiftAmount);

  MachineInstr *ShiftMI = BuildMI(MBB, MI, MI.getDebugLoc(),
                                  TII->get(getStandardShiftOpcode(Opcode)))
                              .add(MI.getOperand(0))
                              .add(MI.getOperand(1));
  constrainSelectedInstRegOperands(*ShiftMI, *TII, *TRI, *RBI);

  MI.removeFromParent();
  return true;
}

bool XtensaShiftLowering::lowerFunnelShift(MachineInstr &MI) {
  unsigned Opcode = MI.getOpcode();
  MachineBasicBlock &MBB = *MI.getParent();
  Register HighInput = MI.getOperand(1).getReg();
  Register LowInput = MI.getOperand(2).getReg();
  Register ShiftAmount = MI.getOperand(3).getReg();

  bool IsLeftShift = Opcode == Xtensa::G_FSHL;

  // The semantics of funnel shifts require masking the shift amount.
  BuildMI(MBB, MI, MI.getDebugLoc(),
          TII->get(IsLeftShift ? Xtensa::G_XTENSA_SSL_MASKED
                               : Xtensa::G_XTENSA_SSR_MASKED))
      .addReg(ShiftAmount);

  MachineInstr *ShiftMI =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(Xtensa::SRC))
          .add(MI.getOperand(0))
          .addReg(HighInput)
          .addReg(LowInput);
  constrainSelectedInstRegOperands(*ShiftMI, *TII, *TRI, *RBI);

  MI.removeFromParent();
  return true;
}

bool XtensaShiftLowering::runOnMachineFunction(MachineFunction &MF) {
  const TargetSubtargetInfo &STI = MF.getSubtarget();

  MRI = &MF.getRegInfo();
  TII = STI.getInstrInfo();
  TRI = STI.getRegisterInfo();
  RBI = STI.getRegBankInfo();

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
