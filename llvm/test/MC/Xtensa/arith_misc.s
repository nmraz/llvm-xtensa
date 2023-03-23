# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: abs a5, a6
# CHECK: encoding: [0x60,0x51,0x60]
abs a5, a6

# Instruction format RRR
# CHECK-INST: sext a2, a3, 7
# CHECK: encoding: [0x00,0x23,0x23]
sext a2, a3, 7

# Instruction format RRR
# CHECK-INST: sext a2, a3, 15
# CHECK: encoding: [0x80,0x23,0x23]
sext a2, a3, 15
