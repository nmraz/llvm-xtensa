include "llvm/Target/Target.td"

include "XtensaRegisterInfo.td"
include "XtensaRegisterBanks.td"
include "XtensaInstrInfo.td"
include "XtensaCallingConv.td"
include "XtensaCombine.td"

def FeatureSalt : SubtargetFeature<"salt", "HasSalt", "true", "Enable salt and "
                                   "saltu instructions">;

def XtensaInstrInfo : InstrInfo;

class Proc<string Name, list<SubtargetFeature> Features>
   : ProcessorModel<Name, NoSchedModel, Features>;

def : Proc<"generic", [FeatureSalt]>;

def XtensaAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = true;
}

def Xtensa : Target {
  let InstructionSet = XtensaInstrInfo;
  let AssemblyParsers = [XtensaAsmParser];
  let AllowRegisterRenaming = true;
}
