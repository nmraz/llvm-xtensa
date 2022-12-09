#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCASMINFO_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class XtensaMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit XtensaMCAsmInfo();
};

} // namespace llvm

#endif
