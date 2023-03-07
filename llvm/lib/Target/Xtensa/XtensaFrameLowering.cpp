#include "XtensaFrameLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/DebugLoc.h"

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

  TII.addConst(MBB, MBB.begin(), DebugLoc(), Xtensa::A1, Xtensa::A1,
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

  TII.addConst(MBB, MBB.getFirstTerminator(), DebugLoc(), Xtensa::A1,
               Xtensa::A1, FrameSize);
}
