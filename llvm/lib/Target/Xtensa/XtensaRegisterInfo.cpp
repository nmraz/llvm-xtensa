#include "XtensaRegisterInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaFrameLowering.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCRegister.h"
#include <cstdint>

using namespace llvm;

#define DEBUG_TYPE "xtensa-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "XtensaGenRegisterInfo.inc"

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
  MachineFunction &MF = *MI.getMF();
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  Register FrameReg = getFrameRegister(MF);
  int FI = MI.getOperand(FIOperandNum).getIndex();
  int64_t AddedOffset = MI.getOperand(FIOperandNum + 1).getImm();
  int64_t RealOffset =
      MFI.getStackSize() + MFI.getObjectOffset(FI) + AddedOffset;

  // TODO: fold into neighboring instruction where possible.
  Register TmpReg = MRI.createVirtualRegister(&Xtensa::GPRRegClass);
  TII.addConst(*II->getParent(), II, II->getDebugLoc(), TmpReg, FrameReg,
               RealOffset);
  MI.getOperand(FIOperandNum).ChangeToRegister(TmpReg, false, false, true);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(0);
}

Register XtensaRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  // TODO: only call0 supports a dedicated frame pointer, a15?
  return Xtensa::A1;
}
