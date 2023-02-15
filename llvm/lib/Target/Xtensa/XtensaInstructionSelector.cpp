//===- XtensaInstructionSelector.cpp -----------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the InstructionSelector class for
/// Xtensa.
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaInstrInfo.h"
#include "XtensaRegisterBankInfo.h"
#include "XtensaSubtarget.h"
#include "XtensaTargetMachine.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

#define DEBUG_TYPE "xtensa-isel"

using namespace llvm;

namespace {

#define GET_GLOBALISEL_PREDICATE_BITSET
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATE_BITSET

class XtensaInstructionSelector : public InstructionSelector {
public:
  XtensaInstructionSelector(const XtensaTargetMachine &TM,
                            const XtensaSubtarget &STI,
                            const XtensaRegisterBankInfo &RBI);

  bool select(MachineInstr &I) override;
  static const char *getName() { return DEBUG_TYPE; }

private:
  /// tblgen generated 'select' implementation that is used as the initial
  /// selector for the patterns that do not require complex C++.
  bool selectImpl(MachineInstr &I, CodeGenCoverage &CoverageInfo) const;

  ComplexRendererFns selectExtuiMaskImm(MachineOperand &Root) const;

  const XtensaInstrInfo &TII;
  const XtensaRegisterInfo &TRI;
  const XtensaRegisterBankInfo &RBI;

#define GET_GLOBALISEL_PREDICATES_DECL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_DECL

#define GET_GLOBALISEL_TEMPORARIES_DECL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_DECL
};

} // end anonymous namespace

#define GET_GLOBALISEL_IMPL
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_IMPL

XtensaInstructionSelector::XtensaInstructionSelector(
    const XtensaTargetMachine &TM, const XtensaSubtarget &STI,
    const XtensaRegisterBankInfo &RBI)
    : TII(*STI.getInstrInfo()), TRI(*STI.getRegisterInfo()), RBI(RBI),
#define GET_GLOBALISEL_PREDICATES_INIT
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_PREDICATES_INIT
#define GET_GLOBALISEL_TEMPORARIES_INIT
#include "XtensaGenGlobalISel.inc"
#undef GET_GLOBALISEL_TEMPORARIES_INIT
{
}

bool XtensaInstructionSelector::select(MachineInstr &I) {
  if (!I.isPreISelOpcode()) {
    // Already target-specific
    return true;
  }

  if (selectImpl(I, *CoverageInfo))
    return true;

  return false;
}

InstructionSelector::ComplexRendererFns
XtensaInstructionSelector::selectExtuiMaskImm(MachineOperand &Root) const {
  auto MaybeConstant = getIConstantVRegValWithLookThrough(
      Root.getReg(), Root.getParent()->getParent()->getParent()->getRegInfo());

  if (!MaybeConstant) {
    return None;
  }

  uint64_t Imm = MaybeConstant->Value.getZExtValue();

  if (Imm > 0xff || !isMask_64(Imm)) {
    return None;
  }

  uint32_t Enc = countTrailingOnes(Imm);
  return {{[=](MachineInstrBuilder &MIB) { MIB.addImm(Enc); }}};
}

namespace llvm {
InstructionSelector *
createXtensaInstructionSelector(const XtensaTargetMachine &TM,
                                const XtensaSubtarget &Subtarget,
                                const XtensaRegisterBankInfo &RBI) {
  return new XtensaInstructionSelector(TM, Subtarget, RBI);
}
} // namespace llvm
