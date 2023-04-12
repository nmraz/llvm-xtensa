# RUN: llvm-mc -triple=xtensa -filetype=obj < %s | llvm-objdump -d - | FileCheck %s

# CHECK: bgez a1, 0x6
# CHECK: j 0x807
bltz a1, label
.space 2049
label:
