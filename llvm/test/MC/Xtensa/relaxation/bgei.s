# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: blti a1, 5, 0x6
# CHECK: j 0x87
bgei a1, 5, label
.space 129
label:
