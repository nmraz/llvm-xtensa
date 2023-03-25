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

#include "XtensaInstrUtils.h"
#include "XtensaRegisterInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstrTypes.h"
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

  bool intCmpRequiresSelect(CmpInst::Predicate Pred) const;
  bool intCmpRequiresBranch(CmpInst::Predicate Pred) const;

  MachineInstr *loadConstWithL32R(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL, Register Dest,
                                  const Constant *Value) const;

  void loadImm(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
               const DebugLoc &DL, Register Dest, int32_t Value) const;

  /// Emits a sequence of instructions adding `Src` and `Parts` into `Dest`
  /// before `I`. The emitted instructions may not be in SSA form.
  void addRegImmParts(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                      const DebugLoc &DL, Register Dest, Register Src,
                      bool KillSrc,
                      const XtensaInstrUtils::AddConstParts &Parts) const;

  /// Emits a sequence of instructions adding `Src` and `Value` into `Dest`
  /// before `I`, by loading the value into a temporary register and using an
  /// ordinary add. The emitted instructions may not be in SSA form.
  void addRegImmWithLoad(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                         const DebugLoc &DL, Register Dest, Register Src,
                         bool KillSrc, Register Temp, int32_t Value) const;

  /// Emits a sequence of instructions adding `Src` and `Value` into `Dest`
  /// before `I`. The emitted instructions may not be in SSA form.
  /// If `AllowSplit` is true, the addition may be split across several add
  /// instructions when deemed beneficial.
  void addRegImm(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                 const DebugLoc &DL, Register Dest, Register Src, bool KillSrc,
                 Register Temp, int32_t Value, bool AllowSplit = true) const;

  unsigned isLoadFromStackSlot(const MachineInstr &MI,
                               int &FrameIndex) const override;
  unsigned isLoadFromStackSlotPostFE(const MachineInstr &MI,
                                     int &FrameIndex) const override;

  unsigned isStoreToStackSlot(const MachineInstr &MI,
                              int &FrameIndex) const override;
  unsigned isStoreToStackSlotPostFE(const MachineInstr &MI,
                                    int &FrameIndex) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

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
