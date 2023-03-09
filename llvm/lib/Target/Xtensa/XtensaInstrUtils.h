#ifndef LLVM_LIB_TARGET_XTENSA_XtensaInstrUtils_H
#define LLVM_LIB_TARGET_XTENSA_XtensaInstrUtils_H

#include "llvm/ADT/Optional.h"
#include <cstdint>

namespace llvm {
namespace XtensaInstrUtils {

struct AddConstParts {
  int8_t Low;
  int16_t Middle;
};

struct OffsetConstParts {
  uint16_t Offset;
  int8_t LowAdd;
  int16_t MiddleAdd;
};

Optional<AddConstParts> splitAddConst(int32_t Value);
Optional<OffsetConstParts> splitOffsetConst(int32_t Value, unsigned Shift);

inline Optional<OffsetConstParts> splitOffsetConst8(int32_t Value) {
  return splitOffsetConst(Value, 0);
}

inline Optional<OffsetConstParts> splitOffsetConst16(int32_t Value) {
  return splitOffsetConst(Value, 1);
}

inline Optional<OffsetConstParts> splitOffsetConst32(int32_t Value) {
  return splitOffsetConst(Value, 2);
}

} // namespace XtensaInstrUtils
} // namespace llvm

#endif
