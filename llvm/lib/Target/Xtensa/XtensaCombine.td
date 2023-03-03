include "llvm/Target/GlobalISel/Combine.td"

def set_sar_lowering_info : GIDefMatchData<"SetSarLoweringInfo">;
def int32_matchinfo : GIDefMatchData<"int32_t">;

def left_identity_zero : GICombineRule<
  (defs root:$root),
  (match (wip_match_opcode G_ADD, G_OR, G_XOR):$root,
    [{ return Helper.matchConstantOp(${root}->getOperand(1), 0); }]),
  (apply [{ return Helper.replaceSingleDefInstWithOperand(*${root}, 2); }])
>;

def lower_set_sar_inrange : GICombineRule<
  (defs root:$root, set_sar_lowering_info:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_INRANGE, G_XTENSA_SSL_INRANGE):$root,
    [{ return matchLowerSetSarInrange(MRI, *Helper.getKnownBits(), *${root}, ${matchinfo}); }]),
  (apply [{ applySetSarLowering(Helper, MRI, *${root}, ${matchinfo}); }])
>;

def set_sar_masked_redundant_and : GICombineRule<
  (defs root:$root, register_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED, G_XTENSA_SSL_MASKED):$root,
    [{ return matchSetSarMaskedRedundantAnd(MRI, *Helper.getKnownBits(), *${root}, ${matchinfo}); }]),
  (apply [{ Helper.replaceRegOpWith(MRI, ${root}->getOperand(0), ${matchinfo}); }])
>;

def invert_set_sar_masked : GICombineRule<
  (defs root:$root, set_sar_lowering_info:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED, G_XTENSA_SSL_MASKED):$root,
    [{ return matchInvSetSarMasked(MRI, *Helper.getKnownBits(), *${root}, ${matchinfo}); }]),
  (apply [{ applySetSarLowering(Helper, MRI, *${root}, ${matchinfo}); }])
>;

def set_sar_masked_const_add_sub_left : GICombineRule<
  (defs root:$root, build_fn_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED, G_XTENSA_SSL_MASKED):$root,
    [{ return matchSetSarMaskedConstAddSub(Helper, MRI, *${root}, 1, ${matchinfo}); }]),
  (apply [{ Helper.applyBuildFnNoErase(*${root}, ${matchinfo}); }])
>;

def set_sar_masked_const_add_sub_right : GICombineRule<
  (defs root:$root, build_fn_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED, G_XTENSA_SSL_MASKED):$root,
    [{ return matchSetSarMaskedConstAddSub(Helper, MRI, *${root}, 2, ${matchinfo}); }]),
  (apply [{ Helper.applyBuildFnNoErase(*${root}, ${matchinfo}); }])
>;

def ssr_masked_const : GICombineRule<
  (defs root:$root, int32_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSR_MASKED):$root,
    [{ return matchUnmaskedConst(MRI, *${root}, ${matchinfo}); }]),
  (apply [{ replaceOperandWithConstant(Helper, MRI, B, ${root}->getOperand(0), ${matchinfo}); }])
>;

def ssl_masked_const : GICombineRule<
  (defs root:$root, int32_matchinfo:$matchinfo),
  (match (wip_match_opcode G_XTENSA_SSL_MASKED):$root,
    [{ return matchSSLMaskedConst(MRI, *${root}, ${matchinfo}); }]),
  (apply [{ applySSLMaskedConst(Helper, MRI, B, *${root}, ${matchinfo}); }])
>;

def XtensaPostLegalizerCombinerHelper : GICombinerHelper<
  "XtensaGenPostLegalizerCombinerHelper",
  [all_combines, left_identity_zero]> {
  let DisableRuleOption = "xtensapostlegalizercombiner-disable-rule";
}

def XtensaShiftCombinerHelper : GICombinerHelper<
  "XtensaGenShiftCombinerHelper",
  [lower_set_sar_inrange, set_sar_masked_redundant_and, invert_set_sar_masked,
   set_sar_masked_const_add_sub_left, set_sar_masked_const_add_sub_right,
   ssr_masked_const, ssl_masked_const, right_identity_zero, left_identity_zero]> {
  let DisableRuleOption = "xtensashiftcombiner-disable-rule";
}
