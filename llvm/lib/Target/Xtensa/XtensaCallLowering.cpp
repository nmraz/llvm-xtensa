#include "XtensaCallLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaISelLowering.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

using namespace llvm;

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

  return false;
}

bool XtensaCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                   CallLoweringInfo &Info) const {
  return false;
}
