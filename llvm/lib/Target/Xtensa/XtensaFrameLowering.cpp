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
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <cstdint>
#include <iterator>

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
  if (!FrameSize && !MFI.adjustsStack()) {
    return;
  }

  MachineBasicBlock::iterator MBBI = MBB.begin();
  adjustStackPointer(TII, *MBBI, -FrameSize);

  if (hasFP(MF)) {
    // Based on empirical evidence from gcc, the frame pointer points to the
    // *end* of the static frame.

    // Advance to just after the last callee save, assuming all CSR saves have
    // been inserted contiguously at the start of the block.
    std::advance(MBBI, MFI.getCalleeSavedInfo().size());
    TII.copyPhysReg(MBB, MBBI, DebugLoc(), Xtensa::A15, Xtensa::A1, false);
  }
}

void XtensaFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();

  if (hasFP(MF)) {
    TII.copyPhysReg(MBB, MBBI, DebugLoc(), Xtensa::A1, Xtensa::A15, true);
  }

  uint64_t FrameSize = MFI.getStackSize();
  if (!FrameSize) {
    return;
  }

  adjustStackPointer(TII, *MBB.getFirstTerminator(), FrameSize);
}

bool XtensaFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects() || TRI->hasStackRealignment(MF);
}

bool XtensaFrameLowering::hasReservedCallFrame(
    const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return !MFI.hasVarSizedObjects();
}

void XtensaFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                               BitVector &SavedRegs,
                                               RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  if (MFI.hasCalls()) {
    SavedRegs.set(Xtensa::A0);
  }

  if (hasFP(MF)) {
    SavedRegs.set(Xtensa::A15);
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
