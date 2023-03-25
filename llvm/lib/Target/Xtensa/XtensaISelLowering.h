#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H

#include "Xtensa.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class XtensaSubtarget;

class XtensaTargetLowering : public TargetLowering {
  const XtensaSubtarget &Subtarget;

public:
  explicit XtensaTargetLowering(const TargetMachine &TM,
                                const XtensaSubtarget &STI);

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;
};
} // namespace llvm

#endif
