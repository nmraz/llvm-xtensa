//===-- XtensaFixupKinds.h - Xtensa Specific Fixup Entries ----------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAFIXUPKINDS_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace Xtensa {
// This table *must* be in the same order of
// MCFixupKindInfo Infos[Xtensa::NumTargetFixupKinds]
// in XtensaAsmBackend.cpp.
enum Fixups {
  // 8-bit PC relative relocation for short-range branches: value added to PC+4
  fixup_xtensa_brtarget8 = FirstTargetFixupKind,

  // 12-bit PC relative relocation for b*z branches: value added to PC+4
  fixup_xtensa_brtarget12,

  // 16-bit negative PC relative relocation for l32r: value added to
  // AlignUp(PC, 4)
  fixup_xtensa_l32rtarget16,

  // 18-bit PC relative relocation for calls and jumps: value added to
  // AlignDown(PC, 4) + 4
  fixup_xtensa_calltarget18,

  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
} // namespace Xtensa
} // namespace llvm

#endif
