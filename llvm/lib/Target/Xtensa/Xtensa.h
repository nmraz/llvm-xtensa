#ifndef LLVM_LIB_TARGET_XTENSA_XTENSA_H
#define LLVM_LIB_TARGET_XTENSA_XTENSA_H

namespace llvm {

class AsmPrinter;
class MCInst;
class MachineInstr;

void LowerXtensaMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                     AsmPrinter &AP);

} // namespace llvm

#endif
