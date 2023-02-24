#include "XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "XtensaInstPrinter.h"
#include "XtensaMCAsmInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include <cstdint>

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "XtensaGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "XtensaGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "XtensaGenRegisterInfo.inc"

uint64_t Xtensa_MC::evaluateBranchTarget(uint64_t Addr, int64_t Imm) {
  return Addr + Imm + 4;
}

uint64_t Xtensa_MC::evaluateL32RTarget(uint64_t Addr, int64_t Imm) {
  return ((Addr + 3) & (uint64_t)-4) + Imm;
}

uint64_t Xtensa_MC::evaluateCallTarget(uint64_t Addr, int64_t Imm) {
  return (Addr & (uint64_t)-4) + Imm + 4;
}

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

static MCSubtargetInfo *
createXtensaMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createXtensaMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCAsmInfo *createXtensaMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options) {
  return new XtensaMCAsmInfo();
}

static MCInstPrinter *createXtensaInstPrinter(const Triple &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  return new XtensaInstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTargetMC() {
  Target &T = getTheXtensaTarget();

  RegisterMCAsmInfoFn X(T, createXtensaMCAsmInfo);

  TargetRegistry::RegisterMCInstrInfo(T, createXtensaMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createXtensaMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createXtensaMCSubtargetInfo);
  TargetRegistry::RegisterMCCodeEmitter(T, createXtensaMCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createXtensaAsmBackend);
  TargetRegistry::RegisterMCInstPrinter(T, createXtensaInstPrinter);

  // TODO: Register these:
  // * MCInstrAnalysis
  // * Streamers
}
