# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bnall a1, a2, 0x6
# CHECK: j 0x87
ball a1, a2, label
.space 129
label:
