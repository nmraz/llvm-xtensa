#include "XtensaTargetTransformInfo.h"
#include "llvm/IR/Intrinsics.h"

using namespace llvm;

bool XtensaTTIImpl::isLoweredToCall(const Function *F) {
  if (!F->isIntrinsic()) {
    return true;
  }

  switch (F->getIntrinsicID()) {
  case Intrinsic::fshl:
  case Intrinsic::fshr:
  case Intrinsic::abs:
  case Intrinsic::smin:
  case Intrinsic::umin:
  case Intrinsic::smax:
  case Intrinsic::umax:
  case Intrinsic::bswap:
  case Intrinsic::sadd_with_overflow:
  case Intrinsic::uadd_with_overflow:
  case Intrinsic::ssub_with_overflow:
  case Intrinsic::usub_with_overflow:
  case Intrinsic::sadd_sat:
  case Intrinsic::uadd_sat:
  case Intrinsic::ssub_sat:
  case Intrinsic::usub_sat:
    return false;
  default:
    return true;
  }
}

void XtensaTTIImpl::getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                                            TTI::UnrollingPreferences &UP,
                                            OptimizationRemarkEmitter *ORE) {
  // Copied out of base implementation, with minor adjustments to enable
  // unrolling even though Xtensa is an in-order CPU.

  // Scan the loop: don't unroll loops with calls.
  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
        if (const Function *F = cast<CallBase>(I).getCalledFunction()) {
          if (!isLoweredToCall(F))
            continue;
        }

        if (ORE) {
          ORE->emit([&]() {
            return OptimizationRemark("TTI", "DontUnroll", L->getStartLoc(),
                                      L->getHeader())
                   << "advising against unrolling the loop because it "
                      "contains a "
                   << ore::NV("Call", &I);
          });
        }
        return;
      }
    }
  }

  // Enable runtime and partial unrolling up to the specified size.
  // Enable using trip count upper bound to unroll loops.
  UP.Partial = UP.Runtime = UP.UpperBound = true;
  UP.DefaultUnrollRuntimeCount = 4;

  // Avoid unrolling when optimizing for size.
  UP.OptSizeThreshold = 0;
  UP.PartialOptSizeThreshold = 0;

  // Xtensa branches are typically a fused compare-and-branch, so assume we
  // remove a single instruction every time we eliminate a back-edge.
  UP.BEInsns = 1;
}
