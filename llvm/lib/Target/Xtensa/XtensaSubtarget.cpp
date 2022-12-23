#include "XtensaSubtarget.h"
#include "XtensaFrameLowering.h"

using namespace llvm;

#define DEBUG_TYPE "xtensa-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "XtensaGenSubtargetInfo.inc"

XtensaSubtarget::XtensaSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                                 const XtensaTargetMachine &TM)
    : XtensaGenSubtargetInfo(TT, CPU, CPU, FS), FrameLowering(*this) {}
