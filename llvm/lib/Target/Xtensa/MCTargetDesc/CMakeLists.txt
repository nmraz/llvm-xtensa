add_llvm_component_library(LLVMXtensaDesc
  XtensaInstPrinter.cpp
  XtensaMCTargetDesc.cpp
  XtensaMCAsmInfo.cpp
  XtensaMCCodeEmitter.cpp
  XtensaELFObjectWriter.cpp
  XtensaAsmBackend.cpp

  LINK_COMPONENTS
  MC
  XtensaInfo
  Support

  ADD_TO_COMPONENT
  Xtensa
)
