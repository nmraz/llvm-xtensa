# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: abs a5, a6
# CHECK: encoding: [0x60,0x51,0x60]
abs a5, a6
