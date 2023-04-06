#include "XtensaRegisterInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaFrameLowering.h"
#include "XtensaInstrInfo.h"
#include "XtensaInstrUtils.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>
#include <cassert>
#include <cstdint>

using namespace llvm;
using namespace XtensaInstrUtils;

#define DEBUG_TYPE "xtensa-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "XtensaGenRegisterInfo.inc"

static unsigned getLoadStoreOffsetShift(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::L8UI:
  case Xtensa::S8I:
    return 0;
  case Xtensa::L16UI:
  case Xtensa::L16SI:
  case Xtensa::S16I:
    return 1;
  case Xtensa::L32I:
  case Xtensa::S32I:
    return 2;
  default:
    llvm_unreachable("Invalid load/store opcode");
  }
}

static void eliminateLoadStoreFrameIndex(MachineInstr &MI,
                                         MachineBasicBlock &MBB,
                                         const XtensaInstrInfo &TII,
                                         MachineRegisterInfo &MRI,
                                         Register FrameReg, int64_t Offset) {
  MachineOperand &BaseOp = MI.getOperand(1);
  MachineOperand &OffsetOp = MI.getOperand(2);
  if (auto Parts =
          splitOffsetConst(Offset, getLoadStoreOffsetShift(MI.getOpcode()))) {
    if (Parts->LowAdd || Parts->MiddleAdd) {
      AddConstParts AddParts = {Parts->LowAdd, Parts->MiddleAdd};
      Register TempReg = MRI.createVirtualRegister(&Xtensa::GPRRegClass);
      TII.addRegImmParts(MBB, MI, MI.getDebugLoc(), TempReg, FrameReg, false,
                         AddParts);
      BaseOp.ChangeToRegister(TempReg, false, false, true);
    } else {
      BaseOp.ChangeToRegister(FrameReg, false);
    }
    OffsetOp.ChangeToImmediate(Parts->Offset);
  } else {
    Register TempReg = MRI.createVirtualRegister(&Xtensa::GPRRegClass);
    TII.addRegImmWithLoad(MBB, MI, MI.getDebugLoc(), TempReg, FrameReg, false,
                          TempReg, Offset);
    BaseOp.ChangeToRegister(TempReg, false, false, true);
    OffsetOp.ChangeToImmediate(0);
  }
}

XtensaRegisterInfo::XtensaRegisterInfo() : XtensaGenRegisterInfo(Xtensa::A0) {}

const MCPhysReg *
XtensaRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_Xtensa_Call0_SaveList;
}

const uint32_t *
XtensaRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                         CallingConv::ID) const {
  return CSR_Xtensa_Call0_RegMask;
}

BitVector XtensaRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Return address
  Reserved.set(Xtensa::A0);
  // Stack pointer
  Reserved.set(Xtensa::A1);

  if (MF.getSubtarget().getFrameLowering()->hasFP(MF)) {
    // Frame pointer
    Reserved.set(Xtensa::A15);
  }

  return Reserved;
}

bool XtensaRegisterInfo::requiresRegisterScavenging(
    const MachineFunction &MF) const {
  return true;
}

bool XtensaRegisterInfo::requiresFrameIndexScavenging(
    const MachineFunction &MF) const {
  return true;
}

void XtensaRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                             int SPAdj, unsigned FIOperandNum,
                                             RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MI.getMF();
  const XtensaSubtarget &STI = MF.getSubtarget<XtensaSubtarget>();
  const XtensaInstrInfo &TII = *STI.getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  int FI = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg = getFrameRegister(MF);

  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();
  bool IsCSRSpill = std::any_of(CSI.begin(), CSI.end(), [&](const auto &CS) {
    return CS.getFrameIdx() == FI;
  });

  // CSR spills/restores always use the stack pointer
  if (IsCSRSpill) {
    FrameReg = Xtensa::A1;
  }

  int64_t AddedOffset = MI.getOperand(FIOperandNum + 1).getImm();
  int64_t RealOffset =
      MFI.getStackSize() + MFI.getObjectOffset(FI) + AddedOffset;

  switch (MI.getOpcode()) {
  case Xtensa::ADDI: {
    Register Dest = MI.getOperand(0).getReg();
    TII.addRegImm(MBB, II, MI.getDebugLoc(), Dest, FrameReg, false, Dest,
                  RealOffset);
    MI.eraseFromParent();
    break;
  }
  case Xtensa::L8UI:
  case Xtensa::L16UI:
  case Xtensa::L16SI:
  case Xtensa::L32I:
  case Xtensa::S8I:
  case Xtensa::S16I:
  case Xtensa::S32I:
    eliminateLoadStoreFrameIndex(MI, MBB, TII, MRI, FrameReg, RealOffset);
    break;
  default:
    llvm_unreachable("Invalid frame index instruction");
  }
}

Register XtensaRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return getFrameLowering(MF)->hasFP(MF) ? Xtensa::A15 : Xtensa::A1;
}
