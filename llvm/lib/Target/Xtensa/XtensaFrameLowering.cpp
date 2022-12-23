#include "XtensaFrameLowering.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

using namespace llvm;

XtensaFrameLowering::XtensaFrameLowering(const XtensaSubtarget &STI)
    : TargetFrameLowering(StackGrowsDown, Align(16), 0), STI(STI) {}

void XtensaFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  // TODO
}

void XtensaFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  // TODO
}
