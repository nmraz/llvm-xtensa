# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bnez a1, 0x6
# CHECK: j 0x807
beqz a1, label
.space 2049
label:
