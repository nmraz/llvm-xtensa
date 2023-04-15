#include "XtensaMulConst.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/Support/DivisionByConstantInfo.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

using namespace llvm;
using namespace Xtensa;

static void bumpCostIf(unsigned &Cost, bool B) { Cost += B; }

bool MulConstPow2Parts::matchFrom(uint64_t AbsMulAmount, bool IsNeg) {
  if (!isPowerOf2_64(AbsMulAmount)) {
    return false;
  }

  ShiftAmount = Log2_64(AbsMulAmount);
  NeedsNeg = IsNeg;
  return true;
}

unsigned MulConstPow2Parts::getCost() const {
  unsigned Cost = 0;
  bumpCostIf(Cost, ShiftAmount > 0); // Need an explicit shift
  bumpCostIf(Cost, NeedsNeg);        // Need an extra `neg`
  return Cost;
}

void MulConstPow2Parts::build(MachineIRBuilder &MIB, Register DestReg,
                              Register InputReg) const {
  LLT S32 = LLT::scalar(32);
  auto Shift = MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, ShiftAmount));
  if (NeedsNeg) {
    MIB.buildSub(DestReg, MIB.buildConstant(S32, 0), Shift);
  } else {
    MIB.buildCopy(DestReg, Shift);
  }
}

bool MulConst2Pow2Parts::matchFrom(uint64_t AbsMulAmount, bool IsNeg) {
  unsigned BaseShiftAmount = countTrailingZeros(AbsMulAmount);
  uint64_t NormalizedMulAmount = AbsMulAmount >> BaseShiftAmount;

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
