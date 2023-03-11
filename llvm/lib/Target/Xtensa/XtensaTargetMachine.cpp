#include "XtensaTargetMachine.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "Xtensa.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/CodeGen/GlobalISel/Localizer.h"
#include "llvm/CodeGen/GlobalISel/RegBankSelect.h"
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
  initializeXtensaPostLegalizerCombinerPass(*PR);
  initializeXtensaShiftLoweringPass(*PR);
  initializeXtensaShiftCombinerPass(*PR);
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
  void addPreRegBankSelect() override;
  bool addRegBankSelect() override;
  void addPreGlobalInstructionSelect() override;
  bool addGlobalInstructionSelect() override;
  void addPreEmitPass() override;
};

} // namespace

TargetPassConfig *XtensaTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new XtensaPassConfig(*this, PM);
}

bool XtensaPassConfig::addIRTranslator() {
  addPass(new IRTranslator(getOptLevel()));
  return false;
}

bool XtensaPassConfig::addLegalizeMachineIR() {
  addPass(new Legalizer());
  return false;
}

void XtensaPassConfig::addPreRegBankSelect() {
  bool EnableOpt = getOptLevel() != CodeGenOpt::None;
  if (EnableOpt) {
    addPass(createXtensaPostLegalizerCombiner());
  }
  addPass(createXtensaShiftLowering());
  if (EnableOpt) {
    addPass(createXtensaShiftCombiner());
  }
}

bool XtensaPassConfig::addRegBankSelect() {
  addPass(new RegBankSelect());
  return false;
}

void XtensaPassConfig::addPreGlobalInstructionSelect() {
  addPass(new Localizer());
}

bool XtensaPassConfig::addGlobalInstructionSelect() {
  addPass(new InstructionSelect(getOptLevel()));
  return false;
}

void XtensaPassConfig::addPreEmitPass() {
  addPass(createXtensaSizeReduction());
}
