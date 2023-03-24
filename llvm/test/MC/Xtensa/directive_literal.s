# RUN: llvm-mc -triple xtensa -filetype obj -o - %s | llvm-readobj -S --sd - | FileCheck %s

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
# CHECK:   Type: SHT_PROGBITS
# CHECK:   Flags [
# CHECK:     SHF_ALLOC
# CHECK:     SHF_EXECINSTR
# CHECK:   ]
# CHECK:   AddressAlignment: 4
# CHECK:   SectionData (
# CHECK:     0000: 78563412
# CHECK:   )
# CHECK: }
