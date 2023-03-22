# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST %s

# Instruction format RRI8
# CHECK-INST: movi a1, -2048
# CHECK: encoding: [0x12,0xa8,0x00]
movi a1, -2048

# Instruction format RRI8
# CHECK-INST: movi a1, 5
# CHECK: encoding: [0x12,0xa0,0x05]
movi a1, 5

# Instruction format RRI8
# CHECK-INST: movi a7, 5
# CHECK: encoding: [0x72,0xa0,0x05]
movi a7, 5

# Instruction format RI7
# CHECK-INST: movi.n a2, 3
# CHECK: encoding: [0x0c,0x32]
movi.n a2, 3
