#include "XtensaMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
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
  assert(false && "relocations not supported yet");
}

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createXtensaELFObjectWriter(const Triple &TT) {
  return std::make_unique<XtensaELFObjectWriter>(0);
}
