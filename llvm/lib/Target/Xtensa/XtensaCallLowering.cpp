#include "XtensaCallLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaISelLowering.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Alignment.h"

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

class XtensaFormalArgHandler : public XtensaIncomingValueHandler {
public:
  XtensaFormalArgHandler(MachineIRBuilder &MIRBuilder, MachineRegisterInfo &MRI)
      : XtensaIncomingValueHandler(MIRBuilder, MRI) {}

  void markPhysRegUsed(unsigned PhysReg) override;
};

class XtensaCallReturnHandler : public XtensaIncomingValueHandler {
  MachineInstrBuilder &MIB;

public:
  XtensaCallReturnHandler(MachineIRBuilder &MIRBuilder,
                          MachineRegisterInfo &MRI, MachineInstrBuilder &MIB)
      : XtensaIncomingValueHandler(MIRBuilder, MRI), MIB(MIB) {}

  void markPhysRegUsed(unsigned PhysReg) override;
};

class XtensaOutgoingValueHandler : public CallLowering::OutgoingValueHandler {
  MachineInstrBuilder &MIB;

public:
  XtensaOutgoingValueHandler(MachineIRBuilder &MIRBuilder,
                             MachineRegisterInfo &MRI, MachineInstrBuilder &MIB)
      : CallLowering::OutgoingValueHandler(MIRBuilder, MRI), MIB(MIB) {}

  Register getStackAddress(uint64_t MemSize, int64_t Offset,
                           MachinePointerInfo &MPO,
                           ISD::ArgFlagsTy Flags) override;

  void assignValueToAddress(Register ValVReg, Register Addr, LLT MemTy,
                            MachinePointerInfo &MPO, CCValAssign &VA) override;

  void assignValueToReg(Register ValVReg, Register PhysReg,
                        CCValAssign VA) override;
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

  MIRBuilder.buildLoad(ValVReg, Addr, *MMO);
}

void XtensaIncomingValueHandler::assignValueToReg(Register ValVReg,
                                                  Register PhysReg,
                                                  CCValAssign VA) {
  markPhysRegUsed(PhysReg);
  CallLowering::IncomingValueHandler::assignValueToReg(ValVReg, PhysReg, VA);
}

void XtensaFormalArgHandler::markPhysRegUsed(unsigned int PhysReg) {
  MIRBuilder.getMRI()->addLiveIn(PhysReg);
  MIRBuilder.getMBB().addLiveIn(PhysReg);
}

void XtensaCallReturnHandler::markPhysRegUsed(unsigned int PhysReg) {
  MIB.addDef(PhysReg, RegState::Implicit);
}

Register XtensaOutgoingValueHandler::getStackAddress(uint64_t MemSize,
                                                     int64_t Offset,
                                                     MachinePointerInfo &MPO,
                                                     ISD::ArgFlagsTy Flags) {
  MachineFunction &MF = MIRBuilder.getMF();
  MPO = MachinePointerInfo::getStack(MF, Offset);

  LLT P0 = LLT::pointer(0, 32);
  LLT S32 = LLT::scalar(32);
  auto SPReg = MIRBuilder.buildCopy(P0, Register(Xtensa::A1));

  auto OffsetReg = MIRBuilder.buildConstant(S32, Offset);
  auto AddrReg = MIRBuilder.buildPtrAdd(P0, SPReg, OffsetReg);
  return AddrReg.getReg(0);
}

void XtensaOutgoingValueHandler::assignValueToAddress(Register ValVReg,
                                                      Register Addr, LLT MemTy,
                                                      MachinePointerInfo &MPO,
                                                      CCValAssign &VA) {
  Register ExtReg = extendRegister(ValVReg, VA);
  MachineMemOperand *MMO = MIRBuilder.getMF().getMachineMemOperand(
      MPO, MachineMemOperand::MOStore, MemTy, Align(4));
  MIRBuilder.buildStore(ExtReg, Addr, *MMO);
}

void XtensaOutgoingValueHandler::assignValueToReg(Register ValVReg,
                                                  Register PhysReg,
                                                  CCValAssign VA) {
  Register ExtReg = extendRegister(ValVReg, VA);
  MIRBuilder.buildCopy(PhysReg, ExtReg);
  MIB.addUse(PhysReg, RegState::Implicit);
}

XtensaCallLowering::XtensaCallLowering(const XtensaTargetLowering &TLI)
    : CallLowering(&TLI) {}

bool XtensaCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                     const Value *Val, ArrayRef<Register> VRegs,
                                     FunctionLoweringInfo &FLI) const {
  MachineInstrBuilder MIB = MIRBuilder.buildInstrNoInsert(Xtensa::RETN);

  if (!VRegs.empty()) {
    auto &MF = MIRBuilder.getMF();
    const auto &F = MF.getFunction();
    auto &DL = MF.getDataLayout();

    SmallVector<ArgInfo, 4> OutRets;

    ArgInfo OrigArgInfo{VRegs, Val->getType(), 0};
    setArgFlags(OrigArgInfo, AttributeList::ReturnIndex, DL, F);
    splitToValueTypes(OrigArgInfo, OutRets, DL, F.getCallingConv());

    OutgoingValueAssigner Assigner(RetCC_Xtensa_Call0);
    XtensaOutgoingValueHandler Handler(MIRBuilder, MF.getRegInfo(), MIB);

    if (!determineAndHandleAssignments(Handler, Assigner, OutRets, MIRBuilder,
                                       F.getCallingConv(), F.isVarArg())) {
      return false;
    }
  }

  MIRBuilder.insertInstr(MIB);
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

  IncomingValueAssigner Assigner(CC_Xtensa_Call0);
  XtensaFormalArgHandler Handler(MIRBuilder, MF.getRegInfo());

  return determineAndHandleAssignments(Handler, Assigner, InArgs, MIRBuilder,
                                       CallConv, F.isVarArg());
}

bool XtensaCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                   CallLoweringInfo &Info) const {
  auto &MF = MIRBuilder.getMF();
  auto &MRI = MF.getRegInfo();
  auto &DL = MF.getDataLayout();

  const auto &F = MF.getFunction();
  auto CallConv = F.getCallingConv();
  bool IsVarArg = F.isVarArg();

  bool IsDirect = !Info.Callee.isReg();

  // Pseudo-instruction to record the total stack space necessary for arguments.
  // The actual adjustment values here will be filled in at the end, once they
  // are known.
  MachineInstrBuilder CallSeqStart =
      MIRBuilder.buildInstr(Xtensa::ADJCALLSTACKDOWN);

  unsigned CallOpcode = IsDirect ? Xtensa::CALL0 : Xtensa::CALLX0;
  MachineInstrBuilder Call = MIRBuilder.buildInstrNoInsert(CallOpcode);
  Call.add(Info.Callee);

  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  Call.addRegMask(TRI->getCallPreservedMask(MF, Info.CallConv));

  SmallVector<ArgInfo, 8> OutArgs;
  for (auto &Arg : Info.OrigArgs) {
    splitToValueTypes(Arg, OutArgs, DL, CallConv);
  }

  OutgoingValueAssigner ArgAssigner(CC_Xtensa_Call0);
  XtensaOutgoingValueHandler ArgHandler(MIRBuilder, MRI, Call);

  if (!determineAndHandleAssignments(ArgHandler, ArgAssigner, OutArgs,
                                     MIRBuilder, CallConv, IsVarArg)) {
    return false;
  }

  // Arguments have been passed, emit the call itself.
  MIRBuilder.insertInstr(Call);

  SmallVector<ArgInfo, 4> InRets;
  splitToValueTypes(Info.OrigRet, InRets, DL, CallConv);

  IncomingValueAssigner RetAssigner(RetCC_Xtensa_Call0);
  XtensaCallReturnHandler RetHandler(MIRBuilder, MRI, Call);

  if (!determineAndHandleAssignments(RetHandler, RetAssigner, InRets,
                                     MIRBuilder, CallConv, IsVarArg)) {
    return false;
  }

  // Now that we know the total adjustment required for argument passing, wrap
  // up the call sequence with the correct value.
  CallSeqStart.addImm(ArgAssigner.StackOffset).addImm(0);
  MIRBuilder.buildInstr(Xtensa::ADJCALLSTACKUP)
      .addImm(ArgAssigner.StackOffset)
      .addImm(0);

  return true;
}
