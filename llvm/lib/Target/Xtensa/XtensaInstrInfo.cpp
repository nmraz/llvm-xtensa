#include "XtensaInstrInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaFrameLowering.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "XtensaGenInstrInfo.inc"

XtensaInstrInfo::XtensaInstrInfo(XtensaSubtarget &ST)
    : XtensaGenInstrInfo(Xtensa::ADJCALLSTACKDOWN, Xtensa::ADJCALLSTACKUP),
      Subtarget(ST) {}
