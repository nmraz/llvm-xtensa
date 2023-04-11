#include "XtensaMCAsmInfo.h"

using namespace llvm;

void XtensaMCAsmInfo::anchor() {}

XtensaMCAsmInfo::XtensaMCAsmInfo() {
  // The "6-byte" instructions are relaxed conditional branches, which actually
  // turn into a branch and a jump.
  MaxInstLength = 6;
}
