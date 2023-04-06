#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"
namespace llvm {

class XtensaFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

  int FPSpillFrameIndex;

public:
  explicit XtensaFunctionInfo(const MachineFunction &MF);

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;

  int getFPSpillFrameIndex() const { return FPSpillFrameIndex; }
  void setFPSpillFrameIndex(int FI) { FPSpillFrameIndex = FI; }
};

} // namespace llvm

#endif
