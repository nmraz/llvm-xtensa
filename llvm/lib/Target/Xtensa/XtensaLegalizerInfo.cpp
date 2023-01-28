#include "XtensaLegalizerInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/Support/LowLevelTypeImpl.h"

using namespace llvm;

XtensaLegalizerInfo::XtensaLegalizerInfo(const XtensaSubtarget &ST) {
  using namespace TargetOpcode;

  const LLT S1 = LLT::scalar(1);
  const LLT S32 = LLT::scalar(32);
  const LLT P0 = LLT::pointer(0, 32);

  getActionDefinitionsBuilder(G_PHI).legalFor({S32, P0}).clampScalar(0, S32,
                                                                     S32);

  getActionDefinitionsBuilder({G_CONSTANT, G_IMPLICIT_DEF, G_BRCOND})
      .legalFor({S32})
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder({G_ADD, G_SUB, G_MUL, G_AND, G_OR, G_XOR})
      .legalFor({S32})
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(
      {G_SADDE, G_SSUBE, G_UADDE, G_USUBE, G_SADDO, G_SSUBO, G_UADDO, G_USUBO})
      .lowerFor({{S32, S1}})
      .widenScalarToNextPow2(0)
      .clampScalar(0, S32, S32);

  getActionDefinitionsBuilder(G_ICMP)
      .legalFor({{S32, S32}, {S32, P0}})
      .clampScalar(0, S32, S32)
      .clampScalar(1, S32, S32);

  // Ext/trunc instructions should all be folded together during legalization,
  // meaning they are never legal in the final output.
  getActionDefinitionsBuilder({G_ZEXT, G_SEXT, G_ANYEXT})
      .legalIf([](const LegalityQuery &Query) { return false; })
      .maxScalar(0, S32);

  getActionDefinitionsBuilder(G_TRUNC)
      .legalIf([](const LegalityQuery &Query) { return false; })
      .maxScalar(1, S32);

  getLegacyLegalizerInfo().computeTables();

  verify(*ST.getInstrInfo());
}
