include "llvm/Target/GlobalISel/Combine.td"

def left_identity_zero : GICombineRule<
  (defs root:$root),
  (match (wip_match_opcode G_ADD, G_OR, G_XOR):$root,
    [{ return Helper.matchConstantOp(${root}->getOperand(1), 0); }]),
  (apply [{ return Helper.replaceSingleDefInstWithOperand(*${root}, 2); }])
>;

def lower_set_sar_inrange_info : GIDefMatchData<"SetSarLoweringInfo">;
def lower_set_sar_inrange : GICombineRule<
  (defs root:$root, lower_set_sar_inrange_info:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_INRANGE, G_XTENSA_SSL_INRANGE):$root,
    [{ return matchLowerSetSarInrange(MRI, *Helper.getKnownBits(), *${root}, ${matchinfo}); }]),
  (apply [{ applyLowerSetSarInrange(Helper, MRI, *${root}, ${matchinfo}); }])
>;

def XtensaPostLegalizerCombinerHelper : GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines, left_identity_zero]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}

def XtensaShiftCombinerHelper : GICombinerHelper<
  "XtensaGenShiftCombinerHelper",
  [lower_set_sar_inrange]> {
  let DisableRuleOption = "xtensashiftcombiner-disable-rule";
}
