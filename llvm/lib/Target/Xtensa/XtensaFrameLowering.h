//===-- XtensaFrameLowering.h - Define frame lowering for Xtensa *- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAFRAMELOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAFRAMELOWERING_H

#include "Xtensa.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class XtensaSubtarget;

class XtensaFrameLowering : public TargetFrameLowering {
protected:
  const XtensaSubtarget &STI;

public:
  explicit XtensaFrameLowering(const XtensaSubtarget &STI);

  bool hasFP(const MachineFunction &MF) const override {
    // TODO: refine this later
    return true;
  }

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
};

} // namespace llvm

#endif
