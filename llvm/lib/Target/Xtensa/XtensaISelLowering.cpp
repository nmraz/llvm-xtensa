#include "XtensaISelLowering.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

XtensaTargetLowering::XtensaTargetLowering(const TargetMachine &TM)
    : TargetLowering(TM) {
  setMinFunctionAlignment(Align(4));
}
