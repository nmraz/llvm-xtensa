# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRR
# CHECK-INST: quou a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xc2]
quou a4, a5, a6

# Instruction format RRR
# CHECK-INST: quos a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xd2]
quos a4, a5, a6

# Instruction format RRR
# CHECK-INST: remu a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xe2]
remu a4, a5, a6

# Instruction format RRR
# CHECK-INST: rems a4, a5, a6
# CHECK: encoding: [0x60,0x45,0xf2]
rems a4, a5, a6
