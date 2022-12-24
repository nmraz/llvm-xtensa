#ifndef LLVM_LIB_TARGET_XTENSA_XTENSACALLLOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSACALLLOWERING_H

#include "llvm/CodeGen/GlobalISel/CallLowering.h"

namespace llvm {

class XtensaTargetLowering;

class XtensaCallLowering : public CallLowering {
public:
  XtensaCallLowering(const XtensaTargetLowering &TLI);

  bool lowerReturn(MachineIRBuilder &MIRBuilder, const Value *Val,
                   ArrayRef<Register> VRegs,
                   FunctionLoweringInfo &FLI) const override;

  bool lowerFormalArguments(MachineIRBuilder &MIRBuilder, const Function &F,
                            ArrayRef<ArrayRef<Register>> VRegs,
                            FunctionLoweringInfo &FLI) const override;

  bool lowerCall(MachineIRBuilder &MIRBuilder,
                 CallLoweringInfo &Info) const override;
};

} // namespace llvm

#endif
