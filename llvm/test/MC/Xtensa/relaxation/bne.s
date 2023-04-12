# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: beq a1, a2, 0x6
# CHECK: j 0x87
bne a1, a2, label
.space 129
label:
