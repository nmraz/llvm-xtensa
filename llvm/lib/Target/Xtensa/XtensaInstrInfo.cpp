#include "XtensaInstrInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaFrameLowering.h"
#include "XtensaInstrUtils.h"
#include "XtensaRegisterInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
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

static bool isUncondBranchOpcode(int Opcode) { return Opcode == Xtensa::J; }

static bool isCondBranchOpcode(int Opcode) {
  switch (Opcode) {
  case Xtensa::BBC:
  case Xtensa::BBCI:
  case Xtensa::BBS:
  case Xtensa::BBSI:
  case Xtensa::BEQ:
  case Xtensa::BEQI:
  case Xtensa::BGE:
  case Xtensa::BGEI:
  case Xtensa::BGEU:
  case Xtensa::BGEUI:
  case Xtensa::BLT:
  case Xtensa::BLTI:
  case Xtensa::BLTU:
  case Xtensa::BLTUI:
  case Xtensa::BNE:
  case Xtensa::BNEI:
  case Xtensa::BEQZ:
  case Xtensa::BGEZ:
  case Xtensa::BLTZ:
  case Xtensa::BNEZ:
    return true;
  }
  return false;
}

static void parseCondBranch(const MachineInstr &MI, MachineBasicBlock *&Target,
                            SmallVectorImpl<MachineOperand> &Cond) {
  unsigned Opcode = MI.getOpcode();

  switch (Opcode) {
  case Xtensa::BBC:
  case Xtensa::BBCI:
  case Xtensa::BBS:
  case Xtensa::BBSI:
  case Xtensa::BEQ:
  case Xtensa::BEQI:
  case Xtensa::BGE:
  case Xtensa::BGEI:
  case Xtensa::BGEU:
  case Xtensa::BGEUI:
  case Xtensa::BLT:
  case Xtensa::BLTI:
  case Xtensa::BLTU:
  case Xtensa::BLTUI:
  case Xtensa::BNE:
  case Xtensa::BNEI:
    Cond.push_back(MachineOperand::CreateImm(Opcode));
    Cond.push_back(MI.getOperand(0));
    Cond.push_back(MI.getOperand(1));
    Target = MI.getOperand(2).getMBB();
    break;

  case Xtensa::BEQZ:
  case Xtensa::BGEZ:
  case Xtensa::BLTZ:
  case Xtensa::BNEZ:
    Cond.push_back(MachineOperand::CreateImm(Opcode));
    Cond.push_back(MI.getOperand(0));
    Target = MI.getOperand(1).getMBB();
    break;
  }
}

static Optional<unsigned> reverseSimpleCondBranch(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::BNE:
    return Xtensa::BEQ;
  case Xtensa::BEQ:
    return Xtensa::BNE;
  case Xtensa::BEQZ:
    return Xtensa::BNEZ;
  case Xtensa::BNEZ:
    return Xtensa::BEQZ;
  }

  return None;
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
  if (I == MBB.end() || !isUnpredicatedTerminator(*I))
    return false;

  if (AllowModify) {
    // If the block ends with an unconditional branch to the fallthrough block,
    // eliminate the branch instruction now. This is also handled by branch
    // folding, but doing it here may reveal additional branches later.
    if (isUncondBranchOpcode(I->getOpcode()) &&
        MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
      I->eraseFromParent();

      // Grab the newly-revealed terminator instruction
      I = MBB.getLastNonDebugInstr();
      if (I == MBB.end() || !isUnpredicatedTerminator(*I))
        return false;
    }
  }

  MachineInstr &LastMI = *I;
  if (I == MBB.begin() || !isUnpredicatedTerminator(*--I)) {
    // The block has only one terminator, analyze it now.
    unsigned Opcode = LastMI.getOpcode();

    if (isUncondBranchOpcode(Opcode)) {
      TBB = LastMI.getOperand(0).getMBB();
      return false;
    }

    if (isCondBranchOpcode(Opcode)) {
      parseCondBranch(LastMI, TBB, Cond);
      return false;
    }

    return true;
  }

  MachineInstr &SecondLastMI = *I;

  if (!isUncondBranchOpcode(LastMI.getOpcode())) {
    return true;
  }

  MachineBasicBlock *UncondMBB = LastMI.getOperand(0).getMBB();

  if (isCondBranchOpcode(SecondLastMI.getOpcode())) {
    parseCondBranch(SecondLastMI, TBB, Cond);
    FBB = UncondMBB;
  } else {
    TBB = UncondMBB;
  }

  return false;
}

unsigned XtensaInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded) const {
  unsigned InstrCount = 1;

  if (Cond.empty()) {
    assert(!FBB &&
           "Unexpected FBB value when constructing unconditional branch");
    BuildMI(&MBB, DL, get(Xtensa::J)).addMBB(TBB);
  } else {
    unsigned Opcode = Cond[0].getImm();

    MachineInstrBuilder MIB = BuildMI(&MBB, DL, get(Opcode));
    for (const MachineOperand &Op : drop_begin(Cond)) {
      MIB.add(Op);
    }
    MIB.addMBB(TBB);

    if (FBB) {
      BuildMI(&MBB, DL, get(Xtensa::J)).addMBB(FBB);
      InstrCount++;
    }
  }

  if (BytesAdded) {
    *BytesAdded = InstrCount * 3;
  }

  return InstrCount;
}

unsigned XtensaInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                       int *BytesRemoved) const {
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  unsigned Opcode = I->getOpcode();
  if (!isUncondBranchOpcode(Opcode) && !isCondBranchOpcode(Opcode)) {
    return 0;
  }

  unsigned InstrCount = 1;

  I->eraseFromParent();
  // Note: we use `end` here to ensure symmetry with `analyzeBranch`, which
  // doesn't skip intervening debug instructions.
  I = MBB.end();

  if (I != MBB.begin()) {
    --I;
    if (isCondBranchOpcode(I->getOpcode())) {
      I->eraseFromParent();
      InstrCount++;
    }
  }

  if (BytesRemoved) {
    *BytesRemoved = InstrCount * 3;
  }

  return InstrCount;
}

bool XtensaInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  MachineOperand &OpcodeOp = Cond[0];

  if (auto ReversedOpcode = reverseSimpleCondBranch(OpcodeOp.getImm())) {
    OpcodeOp.setImm(*ReversedOpcode);
    return false;
  }

  return true;
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
