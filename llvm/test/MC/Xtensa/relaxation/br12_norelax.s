# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bnez a1, 0x803
bnez a1, label
.space 2048
label:
