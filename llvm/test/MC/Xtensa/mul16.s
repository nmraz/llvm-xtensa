# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: mul16u a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xc1]
mul16u a4, a5, a6

# Instruction format RRR
# CHECK-INST: mul16s a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xd1]
mul16s a4, a5, a6
