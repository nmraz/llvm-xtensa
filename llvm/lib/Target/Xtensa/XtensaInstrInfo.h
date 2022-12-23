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
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "XtensaGenInstrInfo.inc"

namespace llvm {

class XtensaSubtarget;

class XtensaInstrInfo : public XtensaGenInstrInfo {
  const XtensaRegisterInfo RI;
  const XtensaSubtarget &Subtarget;

public:
  explicit XtensaInstrInfo(XtensaSubtarget &ST);

  const XtensaRegisterInfo &getRegisterInfo() const { return RI; }
};

} // namespace llvm

#endif
