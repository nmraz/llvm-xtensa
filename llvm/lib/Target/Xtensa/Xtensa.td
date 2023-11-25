include "llvm/Target/Target.td"

include "XtensaRegisterInfo.td"
include "XtensaRegisterBanks.td"
include "XtensaSchedule.td"
include "XtensaInstrInfo.td"
include "XtensaCallingConv.td"
include "XtensaCombine.td"

def FeatureMinMax : SubtargetFeature<"minmax", "HasMinMax", "true", "Enable min"
                                     "and max instructions">;

def FeatureNSA : SubtargetFeature<"nsa", "HasNSA", "true", "Enable nsa and nsau"
                                  "instructions">;

def FeatureSalt : SubtargetFeature<"salt", "HasSalt", "true", "Enable salt and "
                                   "saltu instructions">;

def FeatureLX7 : SubtargetFeature<"lx7", "HasLX7", "true",
                                  "Support LX7 core instructions",
                                  [FeatureSalt]>;

def XtensaInstrInfo : InstrInfo;

include "XtensaSched7Stage.td"

class Proc<string Name, list<SubtargetFeature> Features>
   : ProcessorModel<Name, Xtensa7StageModel, Features>;

def : Proc<"generic", []>;
def : Proc<"esp32", [FeatureMinMax, FeatureNSA]>;
def : Proc<"lx7", [FeatureLX7]>;

def XtensaAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = true;
}

def Xtensa : Target {
  let InstructionSet = XtensaInstrInfo;
  let AssemblyParsers = [XtensaAsmParser];
  let AllowRegisterRenaming = true;
}
