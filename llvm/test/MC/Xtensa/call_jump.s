# RUN: llvm-mc %s -triple=xtensa -show-encoding | FileCheck -check-prefixes=CHECK,CHECK-INST,CHECK-FIXUP %s

.align	4
LBL0:

# Instruction format CALL
# CHECK-INST:  call0   LBL0
# CHECK: encoding: [0bAA000101,A,A]
# CHECK-FIXUP: fixup A - offset: 0, value: LBL0, kind: fixup_xtensa_calltarget18
call0  LBL0

# Instruction format CALLX
# CHECK-INST:  callx0  a1
# CHECK: encoding: [0xc0,0x01,0x00]
callx0 a1

# Instruction format CALL
# CHECK-INST:  j       LBL0
# CHECK: encoding: [0bAA000110,A,A]
# CHECK-FIXUP: fixup A - offset: 0, value: LBL0, kind: fixup_xtensa_jmptarget18
j LBL0

# Instruction format CALLX
# CHECK-INST:  jx      a2
# CHECK: encoding: [0xa0,0x02,0x00]
jx a2

# CHECK-INST: ill
# CHECK: encoding: [0x00,0x00,0x00]
ill

# CHECK-INST: ill.n
# CHECK: encoding: [0x6d,0xf0]
ill.n

# Instruction format CALLX
# CHECK-INST: ret
# CHECK: encoding: [0x80,0x00,0x00]
ret
