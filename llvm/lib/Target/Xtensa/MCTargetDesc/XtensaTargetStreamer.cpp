#include "XtensaTargetStreamer.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

XtensaTargetStreamer::XtensaTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {}

XtensaTargetAsmStreamer::XtensaTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS)
    : XtensaTargetStreamer(S), OS(OS) {}

void XtensaTargetAsmStreamer::emitLiteral(MCSymbol *Name, const MCExpr *Value) {
  const MCAsmInfo *MAI = getStreamer().getContext().getAsmInfo();
  OS << "\t.literal\t";
  Name->print(OS, MAI);
  OS << ", ";
  Value->print(OS, MAI);
  OS << "\n";
}

XtensaTargetELFStreamer::XtensaTargetELFStreamer(MCStreamer &S)
    : XtensaTargetStreamer(S) {}

void XtensaTargetELFStreamer::emitLiteral(MCSymbol *Name, const MCExpr *Value) {
  MCStreamer &S = getStreamer();
  MCContext &Context = S.getContext();
  // TODO: gcc marks the section as AX?
  MCSectionELF *LiteralSection =
      Context.getELFSection(".literal", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);

  S.pushSection();
  S.switchSection(LiteralSection, nullptr);
  S.emitValueToAlignment(4);
  S.emitLabel(Name);
  S.emitValue(Value, 4);
  S.popSection();
}
