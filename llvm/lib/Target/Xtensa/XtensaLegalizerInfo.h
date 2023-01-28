#ifndef LLVM_LIB_TARGET_XTENSA_XTENSALEGALIZERINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSALEGALIZERINFO_H

#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"

namespace llvm {

class XtensaLegalizerInfo : public LegalizerInfo {
public:
  XtensaLegalizerInfo();
};

} // namespace llvm

#endif
