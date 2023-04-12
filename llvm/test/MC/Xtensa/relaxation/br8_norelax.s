# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: beq a1, a2, 0x83
beq a1, a2, label
.space 128
label:
