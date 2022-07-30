include "llvm/Target/Target.td"

include "XtensaRegisterInfo.td"
include "XtensaInstrInfo.td"

def XtensaInstrInfo : InstrInfo;

def Xtensa : Target {
  let InstructionSet = XtensaInstrInfo;
}
