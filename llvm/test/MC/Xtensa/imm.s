# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST,CHECK-FIXUP %s

.LCPI0_0:
.long 1234567

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

# CHECK-INST: l32r a2, .LCPI0_0
# CHECK: encoding: [0x21,A,A]
# CHECK-FIXUP: fixup A - offset: 0, value: .LCPI0_0, kind: fixup_xtensa_l32rtarget16
l32r a2, .LCPI0_0
