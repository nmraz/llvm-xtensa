#ifndef LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H
#define LLVM_LIB_TARGET_XTENSA_XTENSATARGETMACHINE_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class XtensaTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  XtensaTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                      CodeGenOpt::Level OL, bool JIT);
  ~XtensaTargetMachine() override;

  //   const X86Subtarget *getSubtargetImpl(const Function &F) const override;
  // DO NOT IMPLEMENT: There is no such thing as a valid default subtarget,
  // subtargets are per-function entities based on the target-specific
  // attributes of each function.
  //   const X86Subtarget *getSubtargetImpl() const = delete;

  TargetTransformInfo getTargetTransformInfo(const Function &F) const override;

  // Set up the pass pipeline.
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  bool isNoopAddrSpaceCast(unsigned SrcAS, unsigned DestAS) const override {
    return true;
  }
};

} // namespace llvm

#endif
