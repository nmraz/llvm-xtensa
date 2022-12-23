#include "XtensaInstrInfo.h"
#include "XtensaFrameLowering.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "XtensaGenInstrInfo.inc"

XtensaInstrInfo::XtensaInstrInfo(XtensaSubtarget &ST) : Subtarget(ST) {}
