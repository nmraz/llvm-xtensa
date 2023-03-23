# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: mull a4, a5, a6
# CHECK: encoding: [0x60,0x45,0x82]
mull a4, a5, a6

# Instruction format RRR
# CHECK-INST: muluh a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xa2]
muluh a4, a5, a6

# Instruction format RRR
# CHECK-INST: mulsh a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xb2]
mulsh a4, a5, a6
