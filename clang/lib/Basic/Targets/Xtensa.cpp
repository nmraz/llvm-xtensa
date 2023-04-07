//===--- XCore.cpp - Implement XCore target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements XCore TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "Xtensa.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/ADT/STLExtras.h"

using namespace clang;
using namespace clang::targets;

static constexpr llvm::StringLiteral ValidCPUNames[] = {
    {"generic"},
    {"lx7"},
    {"esp32"},
};

bool XtensaTargetInfo::isValidCPUName(StringRef Name) const {
  return llvm::is_contained(ValidCPUNames, Name);
}

void XtensaTargetInfo::getTargetDefines(const LangOptions &Opts,
                                        MacroBuilder &Builder) const {
  Builder.defineMacro("__xtensa__");
}

ArrayRef<Builtin::Info> XtensaTargetInfo::getTargetBuiltins() const {
  return ArrayRef<Builtin::Info>();
}
