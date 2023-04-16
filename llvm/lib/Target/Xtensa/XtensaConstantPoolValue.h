#ifndef LLVM_LIB_TARGET_XTENSA_XTENSACONSTANTPOOLVALUE_H
#define LLVM_LIB_TARGET_XTENSA_XTENSACONSTANTPOOLVALUE_H

#include "llvm/CodeGen/MachineConstantPool.h"

namespace llvm {

class LLVMContext;
class Type;

enum class XtensaCPKind {
  JumpTableAddr,
};

class XtensaConstantPoolValue : public MachineConstantPoolValue {
  XtensaCPKind Kind;

protected:
  XtensaConstantPoolValue(Type *Ty, XtensaCPKind Kind);

public:
  ~XtensaConstantPoolValue();

  bool isJumpTableAddr() const { return Kind == XtensaCPKind::JumpTableAddr; }
};

class XtensaConstantPoolJumpTableAddr : public XtensaConstantPoolValue {
  int JTI;

  explicit XtensaConstantPoolJumpTableAddr(LLVMContext &C, int JTI);

public:
  static XtensaConstantPoolJumpTableAddr *create(LLVMContext &C, int JTI);

  int getJumpTableIndex() const { return JTI; }

  int getExistingMachineCPValue(MachineConstantPool *CP,
                                Align Alignment) override;
  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;
  void print(raw_ostream &O) const override;

  static bool classof(const XtensaConstantPoolValue *XPV) {
    return XPV->isJumpTableAddr();
  }
};

} // namespace llvm

#endif
