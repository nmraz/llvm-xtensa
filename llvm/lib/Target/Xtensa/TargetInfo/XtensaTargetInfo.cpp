#include "XtensaTargetInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheXtensaTarget() {
  static Target TheXtensaTarget;
  return TheXtensaTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetInfo() {
  RegisterTarget<Triple::xtensa> Register(getTheXtensaTarget(), "xtensa",
                                          "Xtensa (little endian)", "Xtensa");
}
