//===-- XtensaInstrInfo.h - Xtensa Instruction Information ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the Xtensa implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAINSTRINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAINSTRINFO_H

#include "XtensaRegisterInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DebugLoc.h"
#include <cstdint>

#define GET_INSTRINFO_HEADER
#include "XtensaGenInstrInfo.inc"

namespace llvm {

class MachineInstr;
class XtensaSubtarget;

class XtensaInstrInfo : public XtensaGenInstrInfo {
  const XtensaRegisterInfo RI;
  const XtensaSubtarget &Subtarget;

public:
  explicit XtensaInstrInfo(XtensaSubtarget &ST);

  const XtensaRegisterInfo &getRegisterInfo() const { return RI; }

  MachineInstr *loadConstWithL32R(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL, Register Dest,
                                  const Constant *Value) const;

  /// Emits a sequence of instructions adding `Src` and `Value` into `Dest`
  /// before `I`. The emitted instructions may not be in SSA form.
  void addRegImm(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, Register Dest, Register Src, bool KillSrc,
                 int32_t Value) const;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, Register SrcReg,
                           bool IsKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;
  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, Register DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;
};

} // namespace llvm

#endif
