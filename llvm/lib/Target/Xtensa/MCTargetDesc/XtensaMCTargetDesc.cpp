#include "XtensaMCTargetDesc.h"
#include "llvm/Support/Compiler.h"

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetMC() {
  // TODO: Register these:
  // * MCAsmInfo
  // * MCInstrInfo
  // * MCRegisterInfo
  // * MCSubtargetInfo
  // * MCInstrAnalysis
  // * MCCodeEmitter
  // * Streamers
}
