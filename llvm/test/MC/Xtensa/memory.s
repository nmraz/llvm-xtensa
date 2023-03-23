# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST,CHECK-FIXUP %s

.align	4
.LCPI0_0:

# Instruction format RRI8
# CHECK-INST: l8ui a2, a1, 3
# CHECK: encoding: [0x22,0x01,0x03]
l8ui a2, sp, 3

# Instruction format RRI8
# CHECK-INST: l16si a3, a1, 4
# CHECK: encoding: [0x32,0x91,0x02]
l16si a3, sp, 4

# Instruction format RRI8
# CHECK-INST: l16ui a4, a1, 6
# CHECK: encoding: [0x42,0x11,0x03]
l16ui a4, sp, 6

# Instruction format RRI8
# CHECK-INST: l32i a5, a1, 8
# CHECK: encoding: [0x52,0x21,0x02]
l32i a5, sp, 8

# Instruction format RRRN
# CHECK-INST: l32i.n a5, a1, 8
# CHECK: encoding: [0x58,0x21]
l32i.n a5, sp, 8

# CHECK-INST: l32r a2, .LCPI0_0
# CHECK: encoding: [0x21,A,A]
# CHECK-FIXUP: fixup A - offset: 0, value: .LCPI0_0, kind: fixup_xtensa_l32rtarget16
l32r a2, .LCPI0_0

# Instruction format RRI8
# CHECK-INST: s8i a2, a1, 3
# CHECK: encoding: [0x22,0x41,0x03]
s8i a2, sp, 3

# Instruction format RRI8
# CHECK-INST: s16i a3, a1, 4
# CHECK: encoding: [0x32,0x51,0x02]
s16i a3, sp, 4

# Instruction format RRI8
# CHECK-INST: s32i a5, a1, 8
# CHECK: encoding: [0x52,0x61,0x02]
s32i a5, sp, 8

# Instruction format RRI8
# CHECK-INST: s32i.n a5, a1, 8
# CHECK: encoding: [0x59,0x21]
s32i.n a5, sp, 8
