#include "XtensaFrameLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaMachineFunctionInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
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
  const XtensaRegisterInfo &TRI = TII.getRegisterInfo();
  const XtensaFunctionInfo &FuncInfo = *MF.getInfo<XtensaFunctionInfo>();
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
    TII.storeRegToStackSlot(MBB, MBBI, Xtensa::A15, true,
                            FuncInfo.getFPSpillFrameIndex(),
                            TRI.getMinimalPhysRegClass(Xtensa::A15), &TRI);
    TII.copyPhysReg(MBB, MBBI, DebugLoc(), Xtensa::A15, Xtensa::A1, false);
  }
}

void XtensaFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  const XtensaRegisterInfo &TRI = TII.getRegisterInfo();
  const XtensaFunctionInfo &FuncInfo = *MF.getInfo<XtensaFunctionInfo>();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();

  if (hasFP(MF)) {
    TII.copyPhysReg(MBB, MBBI, DebugLoc(), Xtensa::A1, Xtensa::A15, true);
    TII.loadRegFromStackSlot(MBB, MBBI, Xtensa::A15,
                             FuncInfo.getFPSpillFrameIndex(),
                             TRI.getMinimalPhysRegClass(Xtensa::A15), &TRI);
  }

  uint64_t FrameSize = MFI.getStackSize();
  if (!FrameSize) {
    return;
  }

  adjustStackPointer(TII, *MBB.getFirstTerminator(), FrameSize);
}

bool XtensaFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    ArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  const MachineFunction &MF = *MBB.getParent();
  const XtensaInstrInfo &TII = *STI.getInstrInfo();

  for (const CalleeSavedInfo &CS : CSI) {
    // Insert the spill to the stack frame.
    Register Reg = CS.getReg();

    if (hasFP(MF) && Reg == Xtensa::A15) {
      // Frame pointer setup will be handled in the prologue.
      continue;
    }

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.storeRegToStackSlot(MBB, MI, Reg, true, CS.getFrameIdx(), RC, TRI);
  }

  return true;
}

bool XtensaFrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    MutableArrayRef<CalleeSavedInfo> CSI, const TargetRegisterInfo *TRI) const {
  const MachineFunction &MF = *MBB.getParent();
  const XtensaInstrInfo &TII = *STI.getInstrInfo();

  for (const CalleeSavedInfo &CI : reverse(CSI)) {
    Register Reg = CI.getReg();

    if (hasFP(MF) && Reg == Xtensa::A15) {
      // Frame poiner teardown will be handled in the epilogue.
      continue;
    }

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.loadRegFromStackSlot(MBB, MI, Reg, CI.getFrameIdx(), RC, TRI);
  }

  return true;
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
  MachineFrameInfo &MFI = MF.getFrameInfo();
  XtensaFunctionInfo &FuncInfo = *MF.getInfo<XtensaFunctionInfo>();

  int SpillSPOffset = -4;
  if (MFI.hasCalls()) {
    SavedRegs.set(Xtensa::A0);
    int FI = MFI.CreateFixedSpillStackObject(4, SpillSPOffset, true);
    FuncInfo.setSpillsRA();
    FuncInfo.setRASpillFrameIndex(FI);
    SpillSPOffset -= 4;
  }

  if (hasFP(MF)) {
    SavedRegs.set(Xtensa::A15);
    int FI = MFI.CreateFixedSpillStackObject(4, SpillSPOffset, true);
    FuncInfo.setFPSpillFrameIndex(FI);
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
