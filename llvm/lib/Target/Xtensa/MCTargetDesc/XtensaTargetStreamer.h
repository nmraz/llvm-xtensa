#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSATARGETSTREAMER_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSATARGETSTREAMER_H

#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/FormattedStream.h"
#include <cstdint>

namespace llvm {

class XtensaTargetStreamer : public MCTargetStreamer {
public:
  XtensaTargetStreamer(MCStreamer &S);
  virtual void emitLiteralPosition() = 0;
  virtual void emitLiteral(MCSymbol *Name, const MCExpr *Value) = 0;
};

class XtensaTargetAsmStreamer : public XtensaTargetStreamer {
  formatted_raw_ostream &OS;

public:
  XtensaTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
  void emitLiteralPosition() override;
  void emitLiteral(MCSymbol *Name, const MCExpr *Value) override;
};

class XtensaTargetELFStreamer : public XtensaTargetStreamer {
public:
  XtensaTargetELFStreamer(MCStreamer &S);
  void emitLiteralPosition() override;
  void emitLiteral(MCSymbol *Name, const MCExpr *Value) override;
};

} // namespace llvm

#endif
