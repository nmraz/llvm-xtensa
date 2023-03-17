#include "XtensaFrameLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

static void adjustStackPointer(const XtensaInstrInfo &TII, MachineInstr &I,
                               int32_t Offset) {
  // Note: we never want to split the stack pointer adjustment, as that may
  // cause it to be temporarily misaligned.
  // The `a8` register is always safe to use as a temporary, as arguments use at
  // most `a7`.
  TII.addRegImm(*I.getParent(), I, DebugLoc(), Xtensa::A1, Xtensa::A1, true,
                Xtensa::A8, Offset, false);
}

XtensaFrameLowering::XtensaFrameLowering(const XtensaSubtarget &STI)
    : TargetFrameLowering(StackGrowsDown, Align(16), 0, Align(16)), STI(STI) {}

void XtensaFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  uint64_t FrameSize = MFI.getStackSize();
  if (!FrameSize) {
    return;
  }

  adjustStackPointer(TII, *MBB.begin(), -FrameSize);
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

  adjustStackPointer(TII, *MBB.getFirstTerminator(), FrameSize);
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
