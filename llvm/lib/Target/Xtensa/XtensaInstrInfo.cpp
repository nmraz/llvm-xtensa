#include "XtensaInstrInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaFrameLowering.h"
#include "XtensaInstrUtils.h"
#include "XtensaRegisterInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace llvm;
using namespace XtensaInstrUtils;

#define GET_INSTRINFO_CTOR_DTOR
#include "XtensaGenInstrInfo.inc"

XtensaInstrInfo::XtensaInstrInfo(XtensaSubtarget &ST)
    : XtensaGenInstrInfo(Xtensa::ADJCALLSTACKDOWN, Xtensa::ADJCALLSTACKUP),
      Subtarget(ST) {}

MachineInstr *XtensaInstrInfo::loadConstWithL32R(MachineBasicBlock &MBB,
                                                 MachineBasicBlock::iterator I,
                                                 const DebugLoc &DL,
                                                 Register Dest,
                                                 const Constant *Value) const {
  MachineFunction &MF = *I->getMF();

  MachineConstantPool *MCP = MF.getConstantPool();

  Align Alignment = Align(4);
  unsigned CPIdx = MCP->getConstantPoolIndex(Value, Alignment);
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getConstantPool(MF), MachineMemOperand::MOLoad,
      LLT::scalar(32), Alignment);

  return BuildMI(MBB, I, DL, get(Xtensa::L32R))
      .addDef(Dest)
      .addConstantPoolIndex(CPIdx)
      .addMemOperand(MMO);
}

void XtensaInstrInfo::addRegImmParts(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I,
                                     const DebugLoc &DL, Register Dest,
                                     Register Src, bool KillSrc,
                                     const AddConstParts &Parts) const {
  if (!Parts.Low && !Parts.Middle) {
    if (Dest != Src) {
      BuildMI(MBB, I, DL, get(Xtensa::MOVN), Dest)
          .addReg(Src, getKillRegState(KillSrc));
    }
    return;
  }

  Register LowSrc = Src;
  bool KillLowSrc = KillSrc;

  if (Parts.Middle) {
    BuildMI(MBB, I, DL, get(Xtensa::ADDMI), Dest)
        .addReg(Src, getKillRegState(KillSrc))
        .addImm(Parts.Middle);
    LowSrc = Dest;
    KillLowSrc = true;
  }

  if (Parts.Low) {
    BuildMI(MBB, I, DL, get(Xtensa::ADDI), Dest)
        .addReg(LowSrc, getKillRegState(KillLowSrc))
        .addImm(Parts.Low);
  }
}

void XtensaInstrInfo::addRegImm(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register Dest, Register Src,
                                bool KillSrc, int32_t Value) const {
  if (auto Parts = splitAddConst(Value)) {
    addRegImmParts(MBB, I, DL, Dest, Src, KillSrc, *Parts);
  } else {
    LLVMContext &Context = MBB.getParent()->getFunction().getContext();
    loadConstWithL32R(MBB, I, DL, Dest,
                      ConstantInt::get(Context, APInt(32, Value)));
    BuildMI(MBB, I, DL, get(Xtensa::ADDN), Dest)
        .addReg(Src, getKillRegState(KillSrc))
        .addReg(Dest, RegState::Kill);
  }
}

void XtensaInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL, MCRegister DestReg,
                                  MCRegister SrcReg, bool KillSrc) const {
  if (Xtensa::GPRRegClass.contains(SrcReg, DestReg)) {
    BuildMI(MBB, I, DL, get(Xtensa::MOVN), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  } else {
    llvm_unreachable("Impossible reg-to-reg copy");
  }
}

void XtensaInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MI,
                                          Register SrcReg, bool IsKill,
                                          int FrameIndex,
                                          const TargetRegisterClass *RC,
                                          const TargetRegisterInfo *TRI) const {
  assert(RC == &Xtensa::GPRRegClass && "Attempted to spill non-GPR to stack");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  DebugLoc DL = MBB.findDebugLoc(MI);

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MI, DL, get(Xtensa::S32I))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}

void XtensaInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
    int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  assert(RC == &Xtensa::GPRRegClass && "Attempted to load non-GPR from stack");

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  DebugLoc DL = MBB.findDebugLoc(MI);

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MI, DL, get(Xtensa::L32I), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}
