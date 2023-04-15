#include "XtensaMulConstHelpers.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/Support/DivisionByConstantInfo.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

using namespace llvm;

static void bumpCostIf(unsigned &Cost, bool B) { Cost += B; }

namespace llvm {
namespace Xtensa {

Optional<PreMulParts> getPreMulParts(uint64_t AbsMulAmount) {
  unsigned ShiftAmount = 0;
  bool NeedsSub = false;
  uint64_t RemainingMulAmount = 0;

  // Attempt to match a pattern that we can create with `addx`/`subx` followed
  // by the generic sum of powers of 2.

  if (AbsMulAmount % 9 == 0) {
    // 9 = 2^3 + 1
    ShiftAmount = 3;
    RemainingMulAmount = AbsMulAmount / 9;
  } else if (AbsMulAmount % 7 == 0) {
    // 7 = 2^3 - 1
    ShiftAmount = 3;
    NeedsSub = true;
    RemainingMulAmount = AbsMulAmount / 7;
  } else if (AbsMulAmount % 5 == 0) {
    // 5 = 2^2 + 1
    ShiftAmount = 2;
    RemainingMulAmount = AbsMulAmount / 5;
  } else if (AbsMulAmount % 3 == 0) {
    // 3 = 2^2 - 1
    ShiftAmount = 1;
    RemainingMulAmount = AbsMulAmount / 3;
  } else {
    return None;
  }

  return {{ShiftAmount, NeedsSub, RemainingMulAmount}};
}

bool MulConst2Pow2Parts::matchFrom(uint64_t AbsMulAmount, bool IsNeg) {
  unsigned BaseShiftAmount = countTrailingZeros(AbsMulAmount);
  uint64_t NormalizedMulAmount = AbsMulAmount >> BaseShiftAmount;

  if (NormalizedMulAmount == 1) {
    // Let other things take care of powers of 2.
    return false;
  }

  if (isPowerOf2_64(NormalizedMulAmount - 1)) {
    // Sum of powers of 2.
    LHSShiftAmount = Log2_64(NormalizedMulAmount - 1) + BaseShiftAmount;
    RHSShiftAmount = BaseShiftAmount;
    NeedsSub = false;
    NeedsNeg = IsNeg;
    return true;
  }

  if (!isPowerOf2_64(NormalizedMulAmount + 1)) {
    return false;
  }

  // Difference of powers of 2.
  unsigned LargeShiftAmount =
      Log2_64(NormalizedMulAmount + 1) + BaseShiftAmount;

  NeedsSub = true;
  NeedsNeg = false;

  if (!IsNeg) {
    LHSShiftAmount = LargeShiftAmount;
    RHSShiftAmount = BaseShiftAmount;
  } else {
    LHSShiftAmount = BaseShiftAmount;
    RHSShiftAmount = LargeShiftAmount;
  }

  return true;
}

static bool canUseAddSubX(unsigned ShiftAmount) { return ShiftAmount <= 3; }

unsigned MulConst2Pow2Parts::getCost() const {
  unsigned Cost = 1; // For the add/sub itself
  if (canUseAddSubX(LHSShiftAmount)) {
    // The LHS shift can be folded into the add/sub, check only the RHS.
    bumpCostIf(Cost, RHSShiftAmount != 0);
  } else if (!NeedsSub && canUseAddSubX(RHSShiftAmount)) {
    // The RHS shift can be folded into the add, check only the LHS.
    bumpCostIf(Cost, LHSShiftAmount != 0);
  } else {
    // We may need an `slli` for each of the shifts.
    bumpCostIf(Cost, LHSShiftAmount != 0);
    bumpCostIf(Cost, RHSShiftAmount != 0);
  }

  bumpCostIf(Cost, NeedsNeg);
  return Cost;
}

void MulConst2Pow2Parts::build(MachineIRBuilder &MIB, Register DestReg,
                               Register InputReg) const {
  LLT S32 = LLT::scalar(32);

  auto LHS =
      MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, LHSShiftAmount));
  auto RHS =
      MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, RHSShiftAmount));
  auto AddSub = MIB.buildInstr(NeedsSub ? Xtensa::G_SUB : Xtensa::G_ADD, {S32},
                               {LHS, RHS});
  if (NeedsNeg) {
    MIB.buildSub(DestReg, MIB.buildConstant(S32, 0), AddSub);
  } else {
    MIB.buildCopy(DestReg, AddSub);
  }
}

} // namespace Xtensa
} // namespace llvm
