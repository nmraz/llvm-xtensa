#include "XtensaFrameLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaRegisterInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLArrayExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <vector>

using namespace llvm;

static void adjustStackPointer(const XtensaInstrInfo &TII, MachineInstr &I,
                               int32_t Offset) {
  // Note: we never want to split the stack pointer adjustment, as that may
  // cause it to be temporarily misaligned.
  MachineRegisterInfo &MRI = I.getParent()->getParent()->getRegInfo();
  Register Temp = MRI.createVirtualRegister(&Xtensa::GPRRegClass);
  TII.addRegImm(*I.getParent(), I, DebugLoc(), Xtensa::A1, Xtensa::A1, true,
                Temp, Offset, false);
}

XtensaFrameLowering::XtensaFrameLowering(const XtensaSubtarget &STI)
    : TargetFrameLowering(StackGrowsDown, Align(16), 0, Align(16)), STI(STI) {}

const TargetFrameLowering::SpillSlot *
XtensaFrameLowering::getCalleeSavedSpillSlots(unsigned int &NumEntries) const {
  static const SpillSlot Slots[] = {{Xtensa::A0, -4}};
  NumEntries = array_lengthof(Slots);
  return Slots;
}

void XtensaFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const XtensaRegisterInfo &TRI = TII.getRegisterInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();

  DebugLoc DL;

  uint64_t FrameSize = MFI.getStackSize();

  MachineBasicBlock::iterator MBBI = MBB.begin();
  adjustStackPointer(TII, *MBBI, -FrameSize);

  // Skip the CSR spills
  std::advance(MBBI, CSI.size());

  if (hasFP(MF)) {
    TII.copyPhysReg(MBB, MBBI, DL, Xtensa::A15, Xtensa::A1, false);

    if (TRI.hasStackRealignment(MF)) {
      Align Alignment = MFI.getMaxAlign();
      Register Temp = MRI.createVirtualRegister(&Xtensa::GPRRegClass);
      TII.loadImm(MBB, MBBI, DL, Temp, -Alignment.value());
      BuildMI(MBB, MBBI, DL, TII.get(Xtensa::AND), Xtensa::A1)
          .addReg(Xtensa::A1, RegState::Kill)
          .addReg(Temp, RegState::Kill);
    }
  }

  if (hasBP(MF)) {
    // The base pointer should point to the stack after realignment, but before
    // any dynamic allocations.
    TII.copyPhysReg(MBB, MBBI, DL, TRI.getBaseRegister(), Xtensa::A1, false);
  }
}

void XtensaFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();

  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();

  if (hasFP(MF)) {
    // Restore the stack based on the frame pointer before all CSR restores.
    MachineBasicBlock::iterator PreRestore = std::prev(MBBI, CSI.size());
    TII.copyPhysReg(MBB, PreRestore, DebugLoc(), Xtensa::A1, Xtensa::A15, true);
  }

  uint64_t FrameSize = MFI.getStackSize();
  adjustStackPointer(TII, *MBBI, FrameSize);
}

bool XtensaFrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects() || TRI->hasStackRealignment(MF);
}

bool XtensaFrameLowering::hasBP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  return MFI.hasVarSizedObjects() && TRI->hasStackRealignment(MF);
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
  MachineFrameInfo &MFI = MF.getFrameInfo();

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
  if (I->getOpcode() == Xtensa::ADJCALLSTACKUP) {
    uint64_t CalleePopAmount = I->getOperand(1).getImm();
    assert(CalleePopAmount == 0 && "Callee pop readjustment is not supported");
  }

  if (!hasReservedCallFrame(MF)) {
    const XtensaInstrInfo &TII =
        *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();

    int32_t Amount = alignTo(I->getOperand(0).getImm(), getStackAlign());
    if (I->getOpcode() == Xtensa::ADJCALLSTACKDOWN)
      Amount = -Amount;

    adjustStackPointer(TII, *I, Amount);
  }

  return MBB.erase(I);
}
