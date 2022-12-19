#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAASMBACKEND_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAASMBACKEND_H

#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmBackend.h"

namespace llvm {

class MCAssembler;
class MCAlignFragment;
struct MCFixupKindInfo;
class MCRegisterInfo;
class Target;

class XtensaAsmBackend : public MCAsmBackend {
  Triple TheTriple;

public:
  XtensaAsmBackend(const Target &T, const MCRegisterInfo &MRI, const Triple &TT,
                   StringRef CPU)
      : MCAsmBackend(support::little), TheTriple(TT) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  Optional<MCFixupKind> getFixupKind(StringRef Name) const override;
  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

  unsigned getNumFixupKinds() const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    return false;
  }

  // Note: we intentionally don't override `getMinimumNopSize`, as that is
  // treated as a NOP size GCD and not as the size of the smallest NOP that can
  // be emitted.

  bool shouldInsertExtraNopBytesForCodeAlign(const MCAlignFragment &AF,
                                             unsigned &Size) override;

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override;
};

} // namespace llvm

#endif
