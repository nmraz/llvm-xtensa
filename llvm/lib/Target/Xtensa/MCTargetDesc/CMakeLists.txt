add_llvm_component_library(LLVMXtensaDesc
  XtensaMCTargetDesc.cpp
  XtensaMCAsmInfo.cpp

  LINK_COMPONENTS
  MC
  XtensaInfo
  Support

  ADD_TO_COMPONENT
  Xtensa
)
