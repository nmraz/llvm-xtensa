#include "XtensaTargetStreamer.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

XtensaTargetStreamer::XtensaTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {}

XtensaTargetAsmStreamer::XtensaTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS)
    : XtensaTargetStreamer(S), OS(OS) {}

void XtensaTargetAsmStreamer::emitLiteral(const MCSymbol *Name,
                                          const MCExpr *Value) {
  const MCAsmInfo *MAI = getStreamer().getContext().getAsmInfo();
  OS << ".literal ";
  Name->print(OS, MAI);
  OS << ", ";
  Value->print(OS, MAI);
  OS << "\n";
}

XtensaTargetELFStreamer::XtensaTargetELFStreamer(MCStreamer &S)
    : XtensaTargetStreamer(S) {}

void XtensaTargetELFStreamer::emitLiteral(const MCSymbol *Name,
                                          const MCExpr *Value) {
  // TODO
}
