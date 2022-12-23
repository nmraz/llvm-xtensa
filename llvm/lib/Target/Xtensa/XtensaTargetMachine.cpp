#include "XtensaTargetMachine.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <memory>

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaTarget() {
  RegisterTargetMachine<XtensaTargetMachine> Register(getTheXtensaTarget());

  PassRegistry *PR = PassRegistry::getPassRegistry();
  initializeGlobalISel(*PR);
}

XtensaTargetMachine::XtensaTargetMachine(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         Optional<Reloc::Model> RM,
                                         Optional<CodeModel::Model> CM,
                                         CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, "e-m:e-p:32:32-i64:64-n32-S128", TT, CPU, FS,
                        Options, Reloc::Static, CodeModel::Small, OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();

  setGlobalISel(true);
}

XtensaTargetMachine::~XtensaTargetMachine() = default;

namespace {

class XtensaPassConfig : public TargetPassConfig {
public:
  XtensaPassConfig(XtensaTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addIRTranslator() override;
  bool addLegalizeMachineIR() override;
  bool addRegBankSelect() override;
  bool addGlobalInstructionSelect() override;
};

} // namespace

TargetPassConfig *XtensaTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new XtensaPassConfig(*this, PM);
}

bool XtensaPassConfig::addIRTranslator() { return false; }
bool XtensaPassConfig::addLegalizeMachineIR() { return false; }
bool XtensaPassConfig::addRegBankSelect() { return false; }
bool XtensaPassConfig::addGlobalInstructionSelect() { return false; }
