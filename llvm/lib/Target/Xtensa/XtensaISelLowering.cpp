#include "XtensaISelLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaRegisterInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/CodeGenPassBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <iterator>

using namespace llvm;

static unsigned getSelectBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::SELECT_LTU:
    return Xtensa::BLTU;
  case Xtensa::SELECT_LTUI:
    return Xtensa::BLTUI;
  case Xtensa::SELECT_GEUI:
    return Xtensa::BGEUI;
  default:
    llvm_unreachable("Unkown branch-select pseudo");
  }
}

XtensaTargetLowering::XtensaTargetLowering(const TargetMachine &TM,
                                           const XtensaSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {
  setMinFunctionAlignment(Align(4));
  setBooleanContents(ZeroOrOneBooleanContent);

  addRegisterClass(MVT::i32, &Xtensa::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());

  MaxStoresPerMemset = 8;
  MaxStoresPerMemsetOptSize = 4;
  MaxStoresPerMemcpy = 4;
  MaxStoresPerMemcpyOptSize = 2;
  MaxStoresPerMemmove = 4;
  MaxStoresPerMemmoveOptSize = 2;
}

MachineBasicBlock *XtensaTargetLowering::EmitInstrWithCustomInserter(
    MachineInstr &MI, MachineBasicBlock *MBB) const {
  unsigned Opcode = MI.getOpcode();
  assert((Opcode == Xtensa::SELECT_LTU || Opcode == Xtensa::SELECT_LTUI ||
          Opcode == Xtensa::SELECT_GEUI) &&
         "Unexpected instruction with custom inserter");

  const TargetInstrInfo &TII = *Subtarget.getInstrInfo();

  const BasicBlock *BB = MBB->getBasicBlock();
  MachineFunction &MF = *MBB->getParent();

  DebugLoc DL = MI.getDebugLoc();

  Register Dest = MI.getOperand(0).getReg();
  const MachineOperand &CmpLHS = MI.getOperand(1);
  const MachineOperand &CmpRHS = MI.getOperand(2);
  Register TrueVal = MI.getOperand(3).getReg();
  Register FalseVal = MI.getOperand(4).getReg();

  // We want to create the following:
  //
  // --------------------------------
  // | ...                          |
  // | {branchcc} lhs, rhs, sinkMBB |
  // --------------------------------
  //    |                     \
  //    |             ----------------- (falseMBB)
  //    |             | (fallthrough) |
  //    |             -----------------
  //    |                     /
  // -------------------------------- (sinkMBB)
  // | dest = phi tval, fval        |
  // | ...                          |
  // --------------------------------

  MachineBasicBlock *FalseMBB = MF.CreateMachineBasicBlock(BB);
  MachineBasicBlock *SinkMBB = MF.CreateMachineBasicBlock(BB);

  MachineFunction::iterator MBBI = std::next(MBB->getIterator());
  MF.insert(MBBI, FalseMBB);
  MF.insert(MBBI, SinkMBB);

  // Move everything after the select to the new basic block
  SinkMBB->splice(SinkMBB->begin(), MBB, std::next(MI.getIterator()),
                  MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);

  MBB->addSuccessor(FalseMBB);
  MBB->addSuccessor(SinkMBB);
  FalseMBB->addSuccessor(SinkMBB);

  BuildMI(MBB, DL, TII.get(getSelectBranchOpcode(Opcode)))
      .add(CmpLHS)
      .add(CmpRHS)
      .addMBB(SinkMBB);

  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(Xtensa::PHI), Dest)
      .addReg(TrueVal)
      .addMBB(MBB)
      .addReg(FalseVal)
      .addMBB(FalseMBB);

  MI.eraseFromParent();

  return SinkMBB;
}
