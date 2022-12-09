#include "XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "XtensaMCAsmInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

static MCAsmInfo *createXtensaMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options) {
  return new XtensaMCAsmInfo();
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetMC() {
  Target &T = getTheXtensaTarget();

  RegisterMCAsmInfoFn X(T, createXtensaMCAsmInfo);

  // TODO: Register these:
  // * MCInstrInfo
  // * MCRegisterInfo
  // * MCSubtargetInfo
  // * MCInstrAnalysis
  // * MCCodeEmitter
  // * Streamers
}
