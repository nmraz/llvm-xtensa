#ifndef LLVM_LIB_TARGET_XTENSA_XtensaInstrUtils_H
#define LLVM_LIB_TARGET_XTENSA_XtensaInstrUtils_H

#include "llvm/ADT/Optional.h"
#include <cstdint>

namespace llvm {
namespace XtensaInstrUtils {

struct AddConstParts {
  int8_t Low;
  int16_t High;
};

Optional<AddConstParts> splitAddConst(int32_t Value);

} // namespace XtensaInstrUtils
} // namespace llvm

#endif
