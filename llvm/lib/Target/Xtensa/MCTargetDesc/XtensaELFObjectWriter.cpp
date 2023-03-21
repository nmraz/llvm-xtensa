#include "XtensaFixupKinds.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>
#include <memory>

using namespace llvm;

namespace {

class XtensaELFObjectWriter : public MCELFObjectTargetWriter {
public:
  XtensaELFObjectWriter(uint8_t OSABI);

  ~XtensaELFObjectWriter() override = default;

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
};

XtensaELFObjectWriter::XtensaELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(false, OSABI, ELF::EM_XTENSA, false) {}

unsigned XtensaELFObjectWriter::getRelocType(MCContext &Ctx,
                                             const MCValue &Target,
                                             const MCFixup &Fixup,
                                             bool IsPCRel) const {
  unsigned Kind = Fixup.getTargetKind();
  if (Kind >= FirstLiteralRelocationKind)
    return Kind - FirstLiteralRelocationKind;

  switch (Kind) {
  default:
    llvm_unreachable("Unsupported relocation kind");
  case FK_NONE:
    return ELF::R_XTENSA_NONE;
  case FK_Data_1:
    Ctx.reportError(Fixup.getLoc(),
                    "Xtensa does not support 1-byte relocations");
    return ELF::R_XTENSA_NONE;
  case FK_Data_2:
    Ctx.reportError(Fixup.getLoc(),
                    "Xtensa does not support 2-byte relocations");
    return ELF::R_XTENSA_NONE;
  case FK_Data_8:
    Ctx.reportError(Fixup.getLoc(),
                    "Xtensa does not support 8-byte relocations");
    return ELF::R_XTENSA_NONE;
  case FK_Data_4:
    assert(!IsPCRel && "Data fixup marked as PCRel");
    return ELF::R_XTENSA_32;
  case Xtensa::fixup_xtensa_brtarget8:
  case Xtensa::fixup_xtensa_brtarget12:
  case Xtensa::fixup_xtensa_l32rtarget16:
  case Xtensa::fixup_xtensa_jmptarget18:
  case Xtensa::fixup_xtensa_calltarget18:
    assert(IsPCRel && "PC-relative fixup marked as non-PCRel");
    return ELF::R_XTENSA_SLOT0_OP;
  }
}

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createXtensaELFObjectWriter(const Triple &TT) {
  return std::make_unique<XtensaELFObjectWriter>(0);
}
