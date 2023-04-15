#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMULCONST_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMULCONST_H

#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/Register.h"
#include <cstdint>

namespace llvm {

class MachineIRBuilder;

namespace Xtensa {

struct PreMulParts {
  unsigned ShiftAmount;
  bool NeedsSub;
  uint64_t RemainingMulAmount;
};

Optional<PreMulParts> getPreMulParts(uint64_t AbsMulAmount);

/// Represents a multiply by a sum or difference of powers of 2, optionally
/// negated.
class MulConst2Pow2Parts {
  unsigned LHSShiftAmount = 0;
  unsigned RHSShiftAmount = 0;
  bool NeedsSub = false;
  bool NeedsNeg = false;

public:
  bool matchFrom(uint64_t AbsMulAmount, bool IsNeg);
  unsigned getCost() const;
  void build(MachineIRBuilder &MIB, Register DestReg, Register InputReg) const;
};

} // namespace Xtensa
} // namespace llvm

#endif
