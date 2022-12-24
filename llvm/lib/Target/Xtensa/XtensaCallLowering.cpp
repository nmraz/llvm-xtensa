#include "XtensaCallLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaISelLowering.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

using namespace llvm;

#include "XtensaGenCallingConv.inc"

namespace {

class XtensaIncomingValueHandler : public CallLowering::IncomingValueHandler {
public:
  XtensaIncomingValueHandler(MachineIRBuilder &MIRBuilder,
                             MachineRegisterInfo &MRI)
      : CallLowering::IncomingValueHandler(MIRBuilder, MRI) {}

  Register getStackAddress(uint64_t MemSize, int64_t Offset,
                           MachinePointerInfo &MPO,
                           ISD::ArgFlagsTy Flags) override;

  void assignValueToAddress(Register ValVReg, Register Addr, LLT MemTy,
                            MachinePointerInfo &MPO, CCValAssign &VA) override;

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign VA) override;

  virtual void markPhysRegUsed(unsigned PhysReg) = 0;
};

class FormalArgHandler : public XtensaIncomingValueHandler {
public:
  FormalArgHandler(MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI)
      : XtensaIncomingValueHandler(MIRBuilder, MRI) {}

  void markPhysRegUsed(unsigned PhysReg) override;
};

} // namespace

Register XtensaIncomingValueHandler::getStackAddress(uint64_t MemSize,
                                                     int64_t Offset,
                                                     MachinePointerInfo &MPO,
                                                     ISD::ArgFlagsTy Flags) {
  MachineFunction &MF = MIRBuilder.getMF();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  // Byval is assumed to be writable memory, but other stack passed arguments
  // are not.
  int FI = MFI.CreateFixedObject(MemSize, Offset, !Flags.isByVal());
  MPO = MachinePointerInfo::getFixedStack(MF, FI);

  return MIRBuilder.buildFrameIndex(LLT::pointer(0, 32), FI).getReg(0);
}

void XtensaIncomingValueHandler::assignValueToAddress(Register ValVReg,
                                                      Register Addr, LLT MemTy,
                                                      MachinePointerInfo &MPO,
                                                      CCValAssign &VA) {
  MachineFunction &MF = MIRBuilder.getMF();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MPO, MachineMemOperand::MOLoad, MemTy, inferAlignFromPtrInfo(MF, MPO));

  // TODO: we need to truncate if the argument was extended to 32 bits
  MIRBuilder.buildLoad(ValVReg, Addr, *MMO);
}

void XtensaIncomingValueHandler::assignValueToReg(Register ValVReg,
                                                  Register PhysReg,
                                                  CCValAssign VA) {
  markPhysRegUsed(PhysReg);
  CallLowering::IncomingValueHandler::assignValueToReg(ValVReg, PhysReg, VA);
}

void FormalArgHandler::markPhysRegUsed(unsigned int PhysReg) {
  MIRBuilder.getMRI()->addLiveIn(PhysReg);
  MIRBuilder.getMBB().addLiveIn(PhysReg);
}

XtensaCallLowering::XtensaCallLowering(const XtensaTargetLowering &TLI)
    : CallLowering(&TLI) {}

bool XtensaCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                     const Value *Val, ArrayRef<Register> VRegs,
                                     FunctionLoweringInfo &FLI) const {
  if (!VRegs.empty()) {
    // TODO: handle return values
    return false;
  }

  MIRBuilder.buildInstr(Xtensa::RETN);
  return true;
}

bool XtensaCallLowering::lowerFormalArguments(
    MachineIRBuilder &MIRBuilder, const Function &F,
    ArrayRef<ArrayRef<Register>> VRegs, FunctionLoweringInfo &FLI) const {
  if (F.arg_empty()) {
    return true;
  }

  auto &MF = MIRBuilder.getMF();
  auto &DL = MF.getDataLayout();

  auto CallConv = F.getCallingConv();

  SmallVector<ArgInfo, 8> InArgs;
  unsigned Idx = 0;
  for (auto &Arg : F.args()) {
    ArgInfo OrigArgInfo{VRegs[Idx], Arg.getType(), Idx};
    setArgFlags(OrigArgInfo, Idx + AttributeList::FirstArgIndex, DL, F);
    splitToValueTypes(OrigArgInfo, InArgs, DL, CallConv);
    Idx++;
  }

  OutgoingValueAssigner Assigner(CC_Xtensa_Call0);
  FormalArgHandler Handler(MIRBuilder, *MIRBuilder.getMRI());

  return determineAndHandleAssignments(Handler, Assigner, InArgs, MIRBuilder,
                                       CallConv, F.isVarArg());
}

bool XtensaCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                   CallLoweringInfo &Info) const {
  return false;
}
