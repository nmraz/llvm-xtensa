# RUN: llvm-mc %s -filetype=obj -triple=xtensa | llvm-readobj -h - | FileCheck %s

# CHECK: Format: elf32-xtensa
# CHECK: Arch: xtensa
# CHECK: AddressSize: 32bit
# CHECK: ElfHeader {
# CHECK:   Ident {
# CHECK:     Magic: (7F 45 4C 46)
# CHECK:     Class: 32-bit (0x1)
# CHECK:     DataEncoding: LittleEndian (0x1)
# CHECK:     FileVersion: 1
# CHECK:     OS/ABI: SystemV (0x0)
# CHECK:     ABIVersion: 0
# CHECK:     Unused: (00 00 00 00 00 00 00)
# CHECK:   }
# CHECK:   Type: Relocatable (0x1)
# CHECK:   Machine: EM_XTENSA (0x5E)
# CHECK:   Version: 1
# CHECK:   Entry: 0x0
# CHECK:   ProgramHeaderOffset: 0x0
# CHECK:   SectionHeaderOffset: 0x5C
# CHECK:   Flags [ (0x0)
# CHECK:   ]
# CHECK:   HeaderSize: 52
# CHECK:   ProgramHeaderEntrySize: 0
# CHECK:   ProgramHeaderCount: 0
# CHECK:   SectionHeaderEntrySize: 40
# CHECK:   SectionHeaderCount: 4
# CHECK:   StringTableSectionIndex: 1
# CHECK: }
