# RUN: not llvm-mc -triple=xtensa -filetype=obj < %s 2>&1 | FileCheck %s

# CHECK:      error: out of range jmptarget18 fixup
j label
.space 262145
label:
