//===-- XtensaBaseInfo.h - Top level definitions for Xtensa MC ---*- C++ -*===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the Xtensa target useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSABASEINFO_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSABASEINFO_H

#include "XtensaMCTargetDesc.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>

namespace llvm {

namespace XtensaII {

inline Optional<uint64_t> encodeB4Const(int64_t Value) {
  switch (Value) {
  case -1:
    return 0;
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    return Value;
  case 10:
    return 9;
  case 12:
    return 10;
  case 16:
    return 11;
  case 32:
    return 12;
  case 64:
    return 13;
  case 128:
    return 14;
  case 256:
    return 15;
  default:
    return None;
  }
}

inline Optional<uint64_t> encodeB4ConstU(uint64_t Value) {
  switch (Value) {
  case 32768:
    return 0;
  case 65536:
    return 1;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    return Value;
  case 10:
    return 9;
  case 12:
    return 10;
  case 16:
    return 11;
  case 32:
    return 12;
  case 64:
    return 13;
  case 128:
    return 14;
  case 256:
    return 15;
  default:
    return None;
  }
}

inline bool isMoviNImm7(int32_t Value) { return Value >= -32 && Value <= 95; }

} // namespace XtensaII

} // namespace llvm

#endif
