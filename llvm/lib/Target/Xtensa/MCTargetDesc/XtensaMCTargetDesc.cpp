#include "XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "XtensaMCAsmInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "XtensaGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "XtensaGenRegisterInfo.inc"

static MCInstrInfo *createXtensaMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitXtensaMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createXtensaMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitXtensaMCRegisterInfo(X, Xtensa::A0);
  return X;
}

static MCAsmInfo *createXtensaMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options) {
  return new XtensaMCAsmInfo();
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetMC() {
  Target &T = getTheXtensaTarget();

  RegisterMCAsmInfoFn X(T, createXtensaMCAsmInfo);

  TargetRegistry::RegisterMCRegInfo(T, createXtensaMCRegisterInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createXtensaMCInstrInfo);

  // TODO: Register these:
  // * MCSubtargetInfo
  // * MCInstrAnalysis
  // * MCCodeEmitter
  // * Streamers
}
