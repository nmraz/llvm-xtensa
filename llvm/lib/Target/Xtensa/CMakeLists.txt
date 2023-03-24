add_llvm_component_group(Xtensa)

set(LLVM_TARGET_DEFINITIONS Xtensa.td)

tablegen(LLVM XtensaGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM XtensaGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM XtensaGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM XtensaGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM XtensaGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM XtensaGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM XtensaGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM XtensaGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM XtensaGenCallingConv.inc -gen-callingconv)
tablegen(LLVM XtensaGenGlobalISel.inc -gen-global-isel --warn-on-skipped-patterns)
tablegen(LLVM XtensaGenPostLegalizeGICombiner.inc -gen-global-isel-combiner
              -combiners="XtensaPostLegalizerCombinerHelper")
tablegen(LLVM XtensaGenO0PostLegalizeGICombiner.inc -gen-global-isel-combiner
              -combiners="XtensaO0PostLegalizerCombinerHelper")
tablegen(LLVM XtensaGenShiftGICombiner.inc -gen-global-isel-combiner
              -combiners="XtensaShiftCombinerHelper")

# tablegen(LLVM XtensaGenMCPseudoLowering.inc -gen-pseudo-lowering)
# tablegen(LLVM XtensaGenExegesis.inc -gen-exegesis)

add_public_tablegen_target(XtensaCommonTableGen)

add_llvm_target(XtensaCodeGen
  XtensaAsmPrinter.cpp
  XtensaCallLowering.cpp
  XtensaFrameLowering.cpp
  XtensaInstrInfo.cpp
  XtensaInstrUtils.cpp
  XtensaInstructionSelector.cpp
  XtensaISelLowering.cpp
  XtensaLegalizerInfo.cpp
  XtensaMCInstLower.cpp
  XtensaO0PostLegalizerCombiner.cpp
  XtensaPostLegalizerCombiner.cpp
  XtensaRegisterBankInfo.cpp
  XtensaRegisterInfo.cpp
  XtensaSizeReduction.cpp
  XtensaShiftCombiner.cpp
  XtensaShiftLowering.cpp
  XtensaSubtarget.cpp
  XtensaTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target
  XtensaInfo
  XtensaDesc

  ADD_TO_COMPONENT
  Xtensa
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
