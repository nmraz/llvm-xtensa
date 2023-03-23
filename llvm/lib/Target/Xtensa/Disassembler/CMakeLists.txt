add_llvm_component_library(LLVMXtensaDisassembler
  XtensaDisassembler.cpp

  LINK_COMPONENTS
  MCDisassembler
  XtensaInfo
  Support

  ADD_TO_COMPONENT
  Xtensa
)
