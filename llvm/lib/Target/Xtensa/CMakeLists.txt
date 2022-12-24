add_llvm_component_group(Xtensa)

set(LLVM_TARGET_DEFINITIONS Xtensa.td)

tablegen(LLVM XtensaGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM XtensaGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM XtensaGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM XtensaGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM XtensaGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM XtensaGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM XtensaGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM XtensaGenCallingConv.inc -gen-callingconv)

# tablegen(LLVM XtensaGenDAGISel.inc -gen-dag-isel)
# tablegen(LLVM XtensaGenDisassemblerTables.inc -gen-disassembler)
# tablegen(LLVM XtensaGenFastISel.inc -gen-fast-isel)
# tablegen(LLVM XtensaGenGlobalISel.inc -gen-global-isel)
# tablegen(LLVM XtensaGenPostLegalizeGICombiner.inc -gen-global-isel-combiner
#               -combiners="XtensaPostLegalizerCombinerHelper")
# tablegen(LLVM XtensaGenMCPseudoLowering.inc -gen-pseudo-lowering)
# tablegen(LLVM XtensaGenRegisterBank.inc -gen-register-bank)
# tablegen(LLVM XtensaGenExegesis.inc -gen-exegesis)

add_public_tablegen_target(XtensaCommonTableGen)

add_llvm_target(XtensaCodeGen
  XtensaAsmPrinter.cpp
  XtensaCallLowering.cpp
  XtensaFrameLowering.cpp
  XtensaInstrInfo.cpp
  XtensaISelLowering.cpp
  XtensaMCInstLower.cpp
  XtensaRegisterInfo.cpp
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
