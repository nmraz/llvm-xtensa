include "llvm/Target/GlobalISel/Combine.td"

def left_identity_zero : GICombineRule<
  (defs root:$root),
  (match (wip_match_opcode G_ADD, G_OR, G_XOR):$root,
    [{ return Helper.matchConstantOp(${root}->getOperand(1), 0); }]),
  (apply [{ return Helper.replaceSingleDefInstWithOperand(*${root}, 2); }])
>;

def lower_set_sar31_matchdata : GIDefMatchData<"unsigned">;
def lower_set_sar31 : GICombineRule<
  (defs root:$root, lower_set_sar31_matchdata:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SET_SAR31):$root,
    [{ return matchLowerSetSar31(*${root}, MRI, ${matchinfo}); }]),
  (apply [{ Helper.replaceOpcodeWith(*${root}, ${matchinfo}); }])
>;

def set_sar_masked_redundant_mask : GICombineRule<
  (defs root:$root, register_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SET_SAR_MASKED):$root,
    [{ return matchSetSarMaskedRedundantMask(*${root}, MRI, ${matchinfo}); }]),
  (apply [{ Helper.replaceRegOpWith(MRI, ${root}->getOperand(0), ${matchinfo}); }])
>;

def XtensaPostLegalizerCombinerHelper : GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines, left_identity_zero]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}

def XtensaShiftCombinerHelper : GICombinerHelper<
  "XtensaGenShiftCombinerHelper",
  [lower_set_sar31, set_sar_masked_redundant_mask]> {
  let DisableRuleOption = "xtensashiftcombiner-disable-rule";
}
