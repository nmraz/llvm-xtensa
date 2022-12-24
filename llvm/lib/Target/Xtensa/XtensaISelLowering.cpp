#include "XtensaISelLowering.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaRegisterInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

XtensaTargetLowering::XtensaTargetLowering(const TargetMachine &TM,
                                           const XtensaSubtarget &STI)
    : TargetLowering(TM) {
  setMinFunctionAlignment(Align(4));

  addRegisterClass(MVT::i32, &Xtensa::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}
