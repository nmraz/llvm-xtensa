# RUN: not llvm-mc  -triple xtensa < %s 2>&1 | FileCheck %s

LBL0:

# Out of range immediates

# simm8
addi a1, a2, 33000
# CHECK:      error: invalid signed 8-bit immediate

# simm8
addi a1, a2, -33000
# CHECK:      error: invalid signed 8-bit immediate

# uimm4_plus1
extui  a1, a2, 5, 17
# CHECK:      error: expected a constant in the range [1, 16]

# simm8_sl8
addmi a1, a2, 33
# CHECK:      error: invalid shifted 8-bit immediate

# uimm5_sub32
slli a1, a2, 0
# CHECK:      error: expected a constant in the range [1, 31]

# uimm4
srli a1, a2, 16
# CHECK:      error: invalid 4-bit immediate

# uimm5
ssai 32
# CHECK:      error: invalid 5-bit immediate

# uimm8
s8i a1, a2, 300
# CHECK:      error: invalid 8-bit immediate

# uimm8_sl1
l16si a1, a2, 512
# CHECK:      error: invalid shifted 8-bit immediate

# uimm8_sl2
l32i a1, a2, 1024
# CHECK:      error: invalid shifted 8-bit immediate

# b4const
beqi a1, 257, LBL0
# CHECK:      error: invalid b4const operand

# b4constu
bgeui a9, 32000, LBL0
# CHECK:      error: invalid b4constu operand

# Invalid number of operands
addi a1, a2 # CHECK: :[[@LINE]]:1: error: too few operands for instruction
addi a1, a2, 4, 4 # CHECK: :[[@LINE]]:17: error: invalid operand for instruction

# Invalid mnemonics
aaa a10, a12 # CHECK: :[[@LINE]]:1: error: invalid instruction

# Invalid operand types
and sp, a2, 10 # CHECK: :[[@LINE]]:13: error: invalid operand for instruction
addi sp, a1, a2 # CHECK: :[[@LINE]]:14: error: operand must be a constant

# Check invalid register names for different formats
# Instruction format RRR
or r2, sp, a3 # CHECK: :[[@LINE]]:4: error: invalid operand for instruction
and a1, r10, a3 # CHECK: :[[@LINE]]:9: error: invalid operand for instruction
sub a1, sp, a100 # CHECK: :[[@LINE]]:13: error: invalid operand for instruction
# Instruction format RRI8
addi a101, sp, 10 # CHECK: :[[@LINE]]:6: error: invalid operand for instruction
addi a1, r10, 10 # CHECK: :[[@LINE]]:10: error: invalid operand for instruction
# Instruction format BRI12
beqz b1, LBL0 # CHECK: :[[@LINE]]:6: error: invalid operand for instruction
# Instruction format BRI8
bltui r7, 16, LBL0 # CHECK: :[[@LINE]]:7: error: invalid operand for instruction
# Instruction format CALLX
callx0 r10 # CHECK: :[[@LINE]]:8: error: invalid operand for instruction

# Check invalid operands order for different formats
# Instruction format RRI8
addi a1, 10, a2 # CHECK: :[[@LINE]]:14: error: operand must be a constant
# Instruction format BRI12
beqz LBL0, a2 # CHECK: :[[@LINE]]:6: error: invalid operand for instruction
# Instruction format BRI8
bltui 16, a7, LBL0 # CHECK: :[[@LINE]]:11: error: operand must be a constant
bltui a7, LBL0, 16 # CHECK: :[[@LINE]]:11: error: operand must be a constant
