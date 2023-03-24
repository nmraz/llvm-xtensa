#include "XtensaSubtarget.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "Xtensa.h"
#include "XtensaCallLowering.h"
#include "XtensaFrameLowering.h"
#include "XtensaLegalizerInfo.h"
#include "XtensaRegisterBankInfo.h"
#include "XtensaTargetMachine.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/RegisterBankInfo.h"

using namespace llvm;

#define DEBUG_TYPE "xtensa-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "XtensaGenSubtargetInfo.inc"

XtensaSubtarget &
XtensaSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS) {
  if (CPU.empty()) {
    CPU = "generic";
  }

  ParseSubtargetFeatures(CPU, CPU, FS);
  return *this;
}

XtensaSubtarget::XtensaSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                                 const XtensaTargetMachine &TM)
    : XtensaGenSubtargetInfo(TT, CPU, CPU, FS),
      FrameLowering(initializeSubtargetDependencies(CPU, FS)), InstrInfo(*this),
      TLInfo(TM, *this) {
  CallLoweringInfo.reset(new XtensaCallLowering(TLInfo));
  auto *RBI = new XtensaRegisterBankInfo(*getRegisterInfo());
  RegBankInfo.reset(RBI);
  Legalizer.reset(new XtensaLegalizerInfo(*this));
  InstSelector.reset(createXtensaInstructionSelector(TM, *this, *RBI));
}

const CallLowering *XtensaSubtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}

const RegisterBankInfo *XtensaSubtarget::getRegBankInfo() const {
  return RegBankInfo.get();
}

const LegalizerInfo *XtensaSubtarget::getLegalizerInfo() const {
  return Legalizer.get();
}

InstructionSelector *XtensaSubtarget::getInstructionSelector() const {
  return InstSelector.get();
}
