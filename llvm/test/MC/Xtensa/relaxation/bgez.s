# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bltz a1, 0x6
# CHECK: j 0x807
bgez a1, label
.space 2049
label:
