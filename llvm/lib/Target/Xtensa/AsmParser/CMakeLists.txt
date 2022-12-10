add_llvm_component_library(LLVMXtensaAsmParser
  XtensaAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  XtensaDesc
  XtensaInfo
  Support

  ADD_TO_COMPONENT
  Xtensa
)
