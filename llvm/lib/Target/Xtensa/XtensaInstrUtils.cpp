#include "XtensaInstrUtils.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

struct OffsetConstInnerParts {
  uint16_t Offset;
  int16_t MiddleAdd;
};

static Optional<OffsetConstInnerParts>
splitOffsetConstInner(int32_t Value, unsigned int Shift) {
  assert(Shift <= 2 && "Shift amount for offset too large");

  uint32_t ZeroMask = (1 << Shift) - 1;
  uint32_t HighMask = static_cast<uint32_t>(-1) << (Shift + 8);

  if ((Value & ZeroMask) != 0) {
    return None;
  }

  int32_t High = Value & HighMask;
  if (!isInt<16>(High)) {
    return None;
  }

  uint16_t Low = Value - High;

  return {{Low, static_cast<int16_t>(High & 0xff00)}};
}

namespace llvm {
namespace XtensaInstrUtils {

Optional<AddConstParts> splitAddConst(int32_t Value) {
  if (isInt<8>(Value)) {
    return {{static_cast<int8_t>(Value), 0}};
  }

  if (isShiftedInt<8, 8>(Value)) {
    return {{0, static_cast<int16_t>(Value)}};
  }

  // 12-bit immediates fit in a `movi` instruction, anything beyond 16 bits
  // won't be representable as two adds anyway.
  if (isInt<12>(Value) || !isInt<16>(Value)) {
    return None;
  }

  int8_t Low = Value & 0xff;

  // Note: this must be unsigned as we want the increment below to wrap.
  uint8_t High = (Value >> 8) & 0xff;
  if (Low < 0) {
    // If sign-extending the low part would subtract 1 from the high part, make
    // sure to compensate here. Note that this will also behave correctly when
    // `High` is already `0xff`, in which case it will wrap to 0 and allow just
    // the low part to be sign-extended.
    High++;
  }

  return {{Low, static_cast<int16_t>(static_cast<int8_t>(High) << 8)}};
}

Optional<OffsetConstParts> splitOffsetConst(int32_t Value, unsigned Shift) {
  if (auto InnerParts = splitOffsetConstInner(Value, Shift)) {
    return {{InnerParts->Offset, 0, InnerParts->MiddleAdd}};
  }

  if (auto AddParts = splitAddConst(Value)) {
    return {{0, AddParts->Low, AddParts->Middle}};
  }

  return None;
}

} // namespace XtensaInstrUtils
} // namespace llvm
