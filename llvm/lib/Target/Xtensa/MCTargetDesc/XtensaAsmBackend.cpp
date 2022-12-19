#include "XtensaAsmBackend.h"
#include "XtensaFixupKinds.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

using namespace llvm;

template <unsigned N>
static unsigned adjustPCRelFixupValue(const MCFixup &Fixup, int64_t Value,
                                      MCContext &Ctx, StringRef Name) {
  // The branch is relative to PC+4, so account for the computed delta by
  // subtracting 4.
  Value -= 4;

  if (!isInt<N>(Value)) {
    Ctx.reportError(Fixup.getLoc(), "out of range " + Name + " fixup");
    return 0;
  }

  return Value;
}

static unsigned adjustFixupValue(const MCFixup &Fixup, int64_t Value,
                                 MCContext &Ctx) {
  unsigned Kind = Fixup.getKind();

  switch (Kind) {
  case Xtensa::fixup_xtensa_brtarget8:
    return adjustPCRelFixupValue<8>(Fixup, Value, Ctx, "brtarget8");

  case Xtensa::fixup_xtensa_brtarget12:
    return adjustPCRelFixupValue<12>(Fixup, Value, Ctx, "brtarget12");

  case Xtensa::fixup_xtensa_jmptarget18:
    return adjustPCRelFixupValue<18>(Fixup, Value, Ctx, "jmptarget18");

  case Xtensa::fixup_xtensa_l32rtarget16:
    if (Fixup.getOffset() % 4 != 0) {
      // This fixup actually needs to be relative to AlignUp(PC, 4) and not
      // AlignDown(PC, 4). Correct for this by manually adjusting if the fixup
      // location is not aligned.
      Value -= 4;
    }

    if (Value % 4 != 0) {
      Ctx.reportError(Fixup.getLoc(), "unaligned l32rtarget fixup");
      return 0;
    }

    Value /= 4;

    if (Value >= 0 || Value < -0x10000) {
      Ctx.reportError(Fixup.getLoc(), "out of range l32rtarget16 fixup");
      return 0;
    }

    return Value;

  case Xtensa::fixup_xtensa_calltarget18:
    Value -= 4;

    if (Value % 4 != 0) {
      Ctx.reportError(Fixup.getLoc(), "unaligned calltarget18 fixup");
      return 0;
    }

    Value /= 4;

    if (!isInt<18>(Value)) {
      Ctx.reportError(Fixup.getLoc(), "out of range calltarget18 fixup");
      return 0;
    }

    return Value;

  default:
    return Value;
  }
}

std::unique_ptr<MCObjectTargetWriter>
XtensaAsmBackend::createObjectTargetWriter() const {
  return createXtensaELFObjectWriter(TheTriple);
}

void XtensaAsmBackend::applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                                  const MCValue &Target,
                                  MutableArrayRef<char> Data, uint64_t Value,
                                  bool IsResolved,
                                  const MCSubtargetInfo *STI) const {
  Value = adjustFixupValue(Fixup, Value, Asm.getContext());
  if (!Value) {
    return;
  }

  const MCFixupKindInfo &Info = getFixupKindInfo(Fixup.getKind());
  // This could be improved by skipping whole bytes of `TargetOffset`, but it
  // doesn't really matter.
  size_t FixupByteOffset = Fixup.getOffset();
  unsigned FixupByteSize = (Info.TargetOffset + Info.TargetSize + 7) / 8;
  uint64_t FixupMask = ((1ull << Info.TargetSize) - 1);

  uint64_t FixedValue = 0;

  // Read the original value as little-endian
  for (unsigned i = 0; i < FixupByteSize; i++) {
    FixedValue |=
        static_cast<uint64_t>(static_cast<uint8_t>(Data[FixupByteOffset + i]))
        << (8 * i);
  }

  FixedValue |= (Value & FixupMask) << Info.TargetOffset;

  // Write back the value as little-endian
  for (unsigned i = 0; i < FixupByteSize; i++) {
    Data[FixupByteOffset + i] = FixedValue >> (8 * i);
  }
}

Optional<MCFixupKind> XtensaAsmBackend::getFixupKind(StringRef Name) const {
  // This needs to return the fixup kind based on relocation name, *not*
  // internal fixup name.
  return None;
}

unsigned XtensaAsmBackend::getNumFixupKinds() const {
  return Xtensa::NumTargetFixupKinds;
}

const MCFixupKindInfo &
XtensaAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  // This table *must* be in same the order of fixup_* kinds in
  // XtensaFixupKinds.h.
  //
  // name                    offset  bits  flags
  const static MCFixupKindInfo Infos[] = {
      {"fixup_xtensa_brtarget8", 16, 8, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_xtensa_brtarget12", 12, 12, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_xtensa_l32rtarget16", 8, 16,
       MCFixupKindInfo::FKF_IsPCRel |
           MCFixupKindInfo::FKF_IsAlignedDownTo32Bits},
      {"fixup_xtensa_jmptarget18", 6, 18, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_xtensa_calltarget18", 6, 18,
       MCFixupKindInfo::FKF_IsPCRel |
           MCFixupKindInfo::FKF_IsAlignedDownTo32Bits},
  };

  if (Kind >= FirstLiteralRelocationKind) {
    return MCAsmBackend::getFixupKindInfo(FK_NONE);
  }

  if (Kind < FirstTargetFixupKind) {
    return MCAsmBackend::getFixupKindInfo(Kind);
  }

  assert((Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
         "Invalid fixup kind");

  return Infos[Kind - FirstTargetFixupKind];
}

bool XtensaAsmBackend::shouldInsertExtraNopBytesForCodeAlign(
    const MCAlignFragment &AF, unsigned int &Size) {
  if (Size == 1) {
    // We support every byte count except 1, so incrementing the offset by the
    // alignment value once more should be sufficient.
    Size += AF.getAlignment().value();
    return true;
  }

  return false;
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
