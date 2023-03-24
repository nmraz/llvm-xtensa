# RUN: llvm-mc -triple xtensa-esp-elf -filetype obj -o - %s \
# RUN:   | llvm-readobj -S --sd - \
# RUN:   | FileCheck %s

	.text
	.literal_position
	.literal .LCPI0_0, 305419896
	.global	test_literal
	.p2align	2
	.type	test_literal,@function
test_literal:
# %bb.0:
	l32r	a2, .LCPI0_0
	ret.n

# CHECK: Section {
# CHECK:   Name: .literal
# CHECK:   SectionData (
# CHECK:     0000: 78563412
# CHECK:   )
# CHECK: }
