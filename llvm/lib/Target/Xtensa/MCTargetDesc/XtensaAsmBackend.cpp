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
#include <cstdint>
#include <memory>

using namespace llvm;

std::unique_ptr<MCObjectTargetWriter>
XtensaAsmBackend::createObjectTargetWriter() const {
  return createXtensaELFObjectWriter(TheTriple);
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

bool XtensaAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count,
                                    const MCSubtargetInfo *STI) const {
  if (Count == 0) {
    return true;
  }

  if (Count == 1) {
    // Xtensa doesn't do 1-byte NOPs.
    return false;
  }

  uint64_t NarrowCount = 0;

  // Why is this loop guaranteed to terminate? At this point we know that
  // `Count >= 2`. If it is even, at worst the loop will terminate with
  // `Count == 0` as we always subtract 2. If it is odd, we can write it as
  // `2 * n + 1`, yet `n > 0`, so it is effectively `2 * (m + 1) + 1
  // = 2 * m + 3`, and the loop will terminate at 3.
  while (Count % 3 != 0) {
    Count -= 2;
    NarrowCount += 1;
  }

  uint64_t WideCount = Count / 3;

  for (uint64_t i = 0; i < WideCount; i++) {
    OS.write("\xf0\x20\x00", 3);
  }

  for (uint64_t i = 0; i < NarrowCount; i++) {
    OS.write("\x3d\xf0", 2);
  }

  return true;
}

MCAsmBackend *llvm::createXtensaAsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {

  return new XtensaAsmBackend(T, MRI, STI.getTargetTriple(), STI.getCPU());
}
