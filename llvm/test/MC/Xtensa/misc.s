# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: sext a2, a3, 7
# CHECK: encoding: [0x00,0x23,0x23]
sext a2, a3, 7

# Instruction format RRR
# CHECK-INST: sext a2, a3, 15
# CHECK: encoding: [0x80,0x23,0x23]
sext a2, a3, 15

# Instruction format RRR
# CHECK-INST: min a4, a5, a6
# CHECK: encoding: [0x60,0x45,0x43]
min a4, a5, a6

# Instruction format RRR
# CHECK-INST: minu a4, a5, a6
# CHECK: encoding: [0x60,0x45,0x63]
minu a4, a5, a6

# Instruction format RRR
# CHECK-INST: max a4, a5, a6
# CHECK: encoding: [0x60,0x45,0x53]
max a4, a5, a6

# Instruction format RRR
# CHECK-INST: maxu a4, a5, a6
# CHECK: encoding: [0x60,0x45,0x73]
maxu a4, a5, a6
