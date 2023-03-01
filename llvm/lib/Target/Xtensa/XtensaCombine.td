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

def set_sar_masked_redundant_and : GICombineRule<
  (defs root:$root, register_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED, G_XTENSA_SSL_MASKED):$root,
    [{ return matchSetSarMaskedRedundantAnd(MRI, *Helper.getKnownBits(), *${root}, ${matchinfo}); }]),
  (apply [{ Helper.replaceRegOpWith(MRI, ${root}->getOperand(0), ${matchinfo}); }])
>;

def XtensaPostLegalizerCombinerHelper : GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines, left_identity_zero]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}

def XtensaShiftCombinerHelper : GICombinerHelper<
  "XtensaGenShiftCombinerHelper",
  [lower_set_sar_inrange, set_sar_masked_redundant_and]> {
  let DisableRuleOption = "xtensashiftcombiner-disable-rule";
}
