#include "XtensaMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"

using namespace llvm;

void XtensaFunctionInfo::anchor() {}

XtensaFunctionInfo::XtensaFunctionInfo(const MachineFunction &MF) {}

MachineFunctionInfo *XtensaFunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
    const {
  return DestMF.cloneInfo<XtensaFunctionInfo>(*this);
}
