#include "XtensaSubtarget.h"
#include "XtensaCallLowering.h"
#include "XtensaFrameLowering.h"
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

XtensaSubtarget::XtensaSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                                 const XtensaTargetMachine &TM)
    : XtensaGenSubtargetInfo(TT, CPU, CPU, FS), FrameLowering(*this),
      InstrInfo(*this), TLInfo(TM) {
  CallLoweringInfo.reset(new XtensaCallLowering(TLInfo));
}

const CallLowering *XtensaSubtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}

const RegisterBankInfo *XtensaSubtarget::getRegBankInfo() const {
  return nullptr;
}

const LegalizerInfo *XtensaSubtarget::getLegalizerInfo() const {
  return nullptr;
}

InstructionSelector *XtensaSubtarget::getInstructionSelector() const {
  return nullptr;
}
