#ifndef LLVM_LIB_TARGET_XTENSA_XTENSALEGALIZERINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSALEGALIZERINFO_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"

namespace llvm {

class XtensaSubtarget;

class XtensaLegalizerInfo : public LegalizerInfo {
public:
  XtensaLegalizerInfo(const XtensaSubtarget &ST);

  bool legalizeCustom(LegalizerHelper &Helper, MachineInstr &MI) const override;

private:
  bool legalizeConstant(MachineInstr &MI, MachineRegisterInfo &MRI,
                        MachineIRBuilder &MIRBuilder,
                        GISelChangeObserver &Observer) const;
  bool legalizeICmp(MachineInstr &MI, MachineRegisterInfo &MRI,
                    MachineIRBuilder &MIRBuilder,
                    GISelChangeObserver &Observer) const;
  bool legalizeSelect(MachineInstr &MI, MachineRegisterInfo &MRI,
                      MachineIRBuilder &MIRBuilder,
                      GISelChangeObserver &Observer) const;
};

} // namespace llvm

#endif
