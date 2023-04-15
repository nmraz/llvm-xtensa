#include "XtensaMulConst.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/Support/DivisionByConstantInfo.h"
#include "llvm/Support/MathExtras.h"

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

static bool getMulShifts(uint64_t MulAmount, unsigned &BaseShiftAmount,
                         unsigned &SmallShiftAmount) {
  for (unsigned I : {0, 1, 2, 3}) {
    uint64_t AdjustedMulAmount = MulAmount - (1 << I);
    if (isPowerOf2_64(AdjustedMulAmount)) {
      BaseShiftAmount = Log2_64(AdjustedMulAmount);
      SmallShiftAmount = I;
      return true;
    }
  }
  return false;
}

bool MulConstWithAddParts::matchFrom(uint64_t AbsMulAmount, bool IsNeg) {
  // Try to break the amount down into `2^B + 2^S`, where `S <= 3`.
  // This will enable us to use an `addx` instruction for the small amount.
  if (getMulShifts(AbsMulAmount, LHSShiftAmount, RHSShiftAmount)) {
    NeedsSub = false;
    NeedsNeg = IsNeg;
    return true;
  }

  // Try to break the amount down into `2^B - 1`.
  uint64_t AdjustedMulAmount = AbsMulAmount + 1;
  if (!isPowerOf2_64(AdjustedMulAmount)) {
    return false;
  }
  unsigned Shift = Log2_64(AdjustedMulAmount);

  NeedsSub = true;
  NeedsNeg = false;

  if (!IsNeg) {
    LHSShiftAmount = Shift;
    RHSShiftAmount = 0;
  } else {
    LHSShiftAmount = 0;
    RHSShiftAmount = Shift;
  }

  return true;
}

static bool canUseAddSubX(unsigned ShiftAmount) { return ShiftAmount <= 3; }

unsigned MulConstWithAddParts::getCost() const {
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

void MulConstWithAddParts::build(MachineIRBuilder &MIB, Register DestReg,
                                 Register InputReg) const {
  LLT S32 = LLT::scalar(32);

  auto BaseShift =
      MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, LHSShiftAmount));
  Register ExtraAddend =
      RHSShiftAmount
          ? MIB.buildShl(S32, InputReg, MIB.buildConstant(S32, RHSShiftAmount))
                .getReg(0)
          : InputReg;
  auto Add = MIB.buildInstr(NeedsSub ? Xtensa::G_SUB : Xtensa::G_ADD, {S32},
                            {BaseShift, ExtraAddend});
  if (NeedsNeg) {
    MIB.buildSub(DestReg, MIB.buildConstant(S32, 0), Add);
  } else {
    MIB.buildCopy(DestReg, Add);
  }
}
