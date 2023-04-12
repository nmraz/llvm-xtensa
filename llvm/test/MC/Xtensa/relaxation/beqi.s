# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bnei a1, 5, 0x6
# CHECK: j 0x87
beqi a1, 5, label
.space 129
label:
