include "llvm/Target/GlobalISel/Combine.td"

def XtensaPostLegalizerCombinerHelper: GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}
