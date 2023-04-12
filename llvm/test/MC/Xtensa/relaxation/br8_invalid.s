# RUN: not llvm-mc -triple=xtensa -filetype=obj < %s 2>&1 | FileCheck %s

# CHECK:      error: out of range brtarget8 fixup
_beq a1, a2, label
.space 129
label:
