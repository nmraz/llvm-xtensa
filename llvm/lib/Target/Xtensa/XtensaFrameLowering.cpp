#include "XtensaFrameLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/Debug.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

XtensaFrameLowering::XtensaFrameLowering(const XtensaSubtarget &STI)
    : TargetFrameLowering(StackGrowsDown, Align(16), 0), STI(STI) {}

void XtensaFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  uint64_t FrameSize = MFI.getStackSize();
  if (!FrameSize) {
    return;
  }

  TII.addRegImm(MBB, MBB.begin(), DebugLoc(), Xtensa::A1, Xtensa::A1,
                -FrameSize);
}

void XtensaFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  uint64_t FrameSize = MFI.getStackSize();
  if (!FrameSize) {
    return;
  }

  TII.addRegImm(MBB, MBB.getFirstTerminator(), DebugLoc(), Xtensa::A1,
                Xtensa::A1, FrameSize);
}

void XtensaFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                               BitVector &SavedRegs,
                                               RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.hasCalls()) {
    SavedRegs.set(Xtensa::A0);
  }
}

MachineBasicBlock::iterator XtensaFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  assert(!MFI.hasVarSizedObjects() &&
         "Dynamic stack readjustments are unimplemented");

  if (I->getOpcode() == TII.getCallFrameDestroyOpcode()) {
    uint64_t CalleePopAmount = I->getOperand(1).getImm();
    assert(CalleePopAmount == 0 && "Callee pop readjustment is not supported");
  }

  return MBB.erase(I);
}
