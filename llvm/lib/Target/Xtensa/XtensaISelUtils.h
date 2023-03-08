#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAISELUTILS_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAISELUTILS_H

#include "llvm/ADT/Optional.h"
#include <cstdint>

namespace llvm {
namespace XtensaISelUtils {

struct AddConstParts {
  int8_t Low;
  int16_t High;
};

Optional<AddConstParts> splitAddConst(int32_t Value);

} // namespace XtensaISelUtils
} // namespace llvm

#endif
