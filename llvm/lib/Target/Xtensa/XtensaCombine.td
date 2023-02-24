include "llvm/Target/GlobalISel/Combine.td"

def left_identity_zero: GICombineRule<
  (defs root:$root),
  (match (wip_match_opcode G_ADD, G_OR, G_XOR):$root,
    [{ return Helper.matchConstantOp(${root}->getOperand(1), 0); }]),
  (apply [{ return Helper.replaceSingleDefInstWithOperand(*${root}, 2); }])
>;

def XtensaPostLegalizerCombinerHelper: GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines, left_identity_zero]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}
