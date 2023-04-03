#ifndef LLVM_LIB_TARGET_XTENSA_XTENSA_H
#define LLVM_LIB_TARGET_XTENSA_XTENSA_H

#include "llvm/PassRegistry.h"

namespace llvm {

class AsmPrinter;
class FunctionPass;
class InstructionSelector;
class MCInst;
class MachineInstr;
class XtensaRegisterBankInfo;
class XtensaSubtarget;
class XtensaTargetMachine;

void LowerXtensaMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                     AsmPrinter &AP);

InstructionSelector *
createXtensaInstructionSelector(const XtensaTargetMachine &TM,
                                const XtensaSubtarget &Subtarget,
                                const XtensaRegisterBankInfo &RBI);

FunctionPass *createXtensaPreLegalizerCombiner();
FunctionPass *createXtensaPostLegalizerCombiner();
FunctionPass *createXtensaO0PostLegalizerCombiner();
FunctionPass *createXtensaShiftLowering();
FunctionPass *createXtensaShiftCombiner();
FunctionPass *createXtensaSizeReduction();

void initializeXtensaPreLegalizerCombinerPass(PassRegistry &PR);
void initializeXtensaPostLegalizerCombinerPass(PassRegistry &PR);
void initializeXtensaO0PostLegalizerCombinerPass(PassRegistry &PR);
void initializeXtensaShiftLoweringPass(PassRegistry &PR);
void initializeXtensaShiftCombinerPass(PassRegistry &PR);
void initializeXtensaSizeReductionPass(PassRegistry &PR);

} // namespace llvm

#endif
