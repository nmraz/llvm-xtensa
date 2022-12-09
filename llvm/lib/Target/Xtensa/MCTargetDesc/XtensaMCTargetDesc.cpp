#include "XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "XtensaMCAsmInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_REGINFO_MC_DESC
#include "XtensaGenRegisterInfo.inc"

static MCAsmInfo *createXtensaMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options) {
  return new XtensaMCAsmInfo();
}

static MCRegisterInfo *createXtensaMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitXtensaMCRegisterInfo(X, Xtensa::A0);
  return X;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetMC() {
  Target &T = getTheXtensaTarget();

  RegisterMCAsmInfoFn X(T, createXtensaMCAsmInfo);

  TargetRegistry::RegisterMCRegInfo(T, createXtensaMCRegisterInfo);

  // TODO: Register these:
  // * MCInstrInfo
  // * MCSubtargetInfo
  // * MCInstrAnalysis
  // * MCCodeEmitter
  // * Streamers
}
