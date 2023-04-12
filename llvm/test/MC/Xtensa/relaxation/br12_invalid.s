# RUN: not llvm-mc -triple=xtensa -filetype=obj < %s 2>&1 | FileCheck %s

# CHECK:      error: out of range brtarget12 fixup
_bnez a1, label
.space 2049
label:
