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
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>

using namespace llvm;
using namespace XtensaInstrUtils;

#define GET_INSTRINFO_CTOR_DTOR
#include "XtensaGenInstrInfo.inc"

static bool isLoad(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::L8UI:
  case Xtensa::L16UI:
  case Xtensa::L16SI:
  case Xtensa::L32I:
    return true;
  default:
    return false;
  }
}

static bool isStore(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::S8I:
  case Xtensa::S16I:
  case Xtensa::S32I:
    return true;
  default:
    return false;
  }
}

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

void XtensaInstrInfo::loadImm(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I, const DebugLoc &DL,
                              Register Dest, int32_t Value) const {
  if (isInt<12>(Value)) {
    BuildMI(MBB, I, DL, get(Xtensa::MOVI), Dest).addImm(Value);
  } else {
    LLVMContext &Context = MBB.getParent()->getFunction().getContext();
    loadConstWithL32R(MBB, I, DL, Dest,
                      ConstantInt::get(Context, APInt(32, Value)));
  }
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

void XtensaInstrInfo::addRegImmWithLoad(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator I,
                                        const DebugLoc &DL, Register Dest,
                                        Register Src, bool KillSrc,
                                        Register Temp, int32_t Value) const {
  assert(Temp != Src && "Temporary register would clobber source");
  loadImm(MBB, I, DL, Temp, Value);
  BuildMI(MBB, I, DL, get(Xtensa::ADDN), Dest)
      .addReg(Src, getKillRegState(KillSrc))
      .addReg(Temp, RegState::Kill);
}

void XtensaInstrInfo::addRegImm(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register Dest, Register Src,
                                bool KillSrc, Register Temp, int32_t Value,
                                bool AllowSplit) const {
  if (auto Parts = splitAddConst(Value)) {
    if (AllowSplit || !(Parts->Low && Parts->Middle)) {
      addRegImmParts(MBB, I, DL, Dest, Src, KillSrc, *Parts);
      return;
    }
  }
  addRegImmWithLoad(MBB, I, DL, Dest, Src, KillSrc, Temp, Value);
}

unsigned XtensaInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                              int &FrameIndex) const {
  if (isLoad(MI.getOpcode()) && MI.getOperand(1).isFI() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return true;
  }

  return false;
}

unsigned XtensaInstrInfo::isLoadFromStackSlotPostFE(const MachineInstr &MI,
                                                    int &FrameIndex) const {

  SmallVector<const MachineMemOperand *, 1> Accesses;
  if (MI.mayLoad() && hasLoadFromStackSlot(MI, Accesses) &&
      Accesses.size() == 1) {
    FrameIndex =
        cast<FixedStackPseudoSourceValue>(Accesses.front()->getPseudoValue())
            ->getFrameIndex();
    return true;
  }
  return false;
}

unsigned XtensaInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                             int &FrameIndex) const {
  if (isStore(MI.getOpcode()) && MI.getOperand(1).isFI() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return true;
  }

  return false;
}

unsigned XtensaInstrInfo::isStoreToStackSlotPostFE(const MachineInstr &MI,
                                                   int &FrameIndex) const {

  SmallVector<const MachineMemOperand *, 1> Accesses;
  if (MI.mayStore() && hasStoreToStackSlot(MI, Accesses) &&
      Accesses.size() == 1) {
    FrameIndex =
        cast<FixedStackPseudoSourceValue>(Accesses.front()->getPseudoValue())
            ->getFrameIndex();
    return true;
  }
  return false;
}

bool XtensaInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *&TBB,
                                    MachineBasicBlock *&FBB,
                                    SmallVectorImpl<MachineOperand> &Cond,
                                    bool AllowModify) const {
  // If the block has no terminators, it just falls into the block after it.
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return false;

  if (!isUnpredicatedTerminator(*I))
    return false;

  if (AllowModify) {
    // If the BB ends with an unconditional branch to the fallthrough BB,
    // we eliminate the branch instruction.
    if (I->getOpcode() == Xtensa::J &&
        MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
      I->eraseFromParent();

      // We update iterator after deleting the last branch.
      I = MBB.getLastNonDebugInstr();
      if (I == MBB.end() || !isUnpredicatedTerminator(*I))
        return false;
    }
  }

  return true;
}

unsigned XtensaInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  assert(Cond.empty() && "Conditional branches are unimplemented");
  BuildMI(&MBB, DL, get(Xtensa::J)).addMBB(TBB);
  if (BytesAdded) {
    *BytesAdded = 3;
  }
  return 1;
}

unsigned XtensaInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                       int *BytesRemoved) const {
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I->getOpcode() != Xtensa::J) {
    return 0;
  }

  I->eraseFromParent();
  if (BytesRemoved) {
    *BytesRemoved = 3;
  }
  return 1;
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
