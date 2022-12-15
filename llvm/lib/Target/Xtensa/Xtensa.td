include "llvm/Target/Target.td"

include "XtensaRegisterInfo.td"
include "XtensaInstrInfo.td"

def XtensaInstrInfo : InstrInfo;

def XtensaAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = true;
}

def Xtensa : Target {
  let InstructionSet = XtensaInstrInfo;
  let AssemblyParsers = [XtensaAsmParser];
  let AllowRegisterRenaming = true;
}
