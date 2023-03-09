#include "XtensaInstrUtils.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

namespace llvm {
namespace XtensaInstrUtils {

Optional<AddConstParts> splitAddConst(int32_t Value) {
  if (!isInt<16>(Value)) {
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

} // namespace XtensaInstrUtils
} // namespace llvm
