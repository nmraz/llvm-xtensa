#include "XtensaAsmBackend.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include <cassert>
#include <memory>

using namespace llvm;

std::unique_ptr<MCObjectTargetWriter>
XtensaAsmBackend::createObjectTargetWriter() const {
  return nullptr;
}

void XtensaAsmBackend::applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                                  const MCValue &Target,
                                  MutableArrayRef<char> Data, uint64_t Value,
                                  bool IsResolved,
                                  const MCSubtargetInfo *STI) const {
  // TODO
}

Optional<MCFixupKind> XtensaAsmBackend::getFixupKind(StringRef Name) const {
  return None;
}

const MCFixupKindInfo &
XtensaAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  assert(false && "Fixup kind info not implemented!");
}

MCAsmBackend *llvm::createXtensaAsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {

  return new XtensaAsmBackend(T, MRI, STI.getTargetTriple(), STI.getCPU());
}
