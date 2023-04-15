#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMULCONST_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMULCONST_H

#include "llvm/CodeGen/Register.h"

namespace llvm {

class MachineIRBuilder;

namespace Xtensa {

/// Represents a multiply of the form (shl x, C), optionally negated.
struct MulConstPow2Parts {
  unsigned ShiftAmount = 0;
  bool NeedsNeg = false;

public:
  bool matchFrom(uint64_t AbsMulAmount, bool IsNeg);
  unsigned getCost() const;
  void build(MachineIRBuilder &MIB, Register DestReg, Register InputReg) const;
};

/// Represents a multiply of one of the forms:
/// * (add (shl x, B), (shl x, S))
/// * (neg (add (shl x, B), (shl x, S)))
/// * (sub (shl x, B), x)
/// * (sub x, (shl x, B))
///
/// Where `S` is at most 3.
class MulConstWithAddParts {
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
