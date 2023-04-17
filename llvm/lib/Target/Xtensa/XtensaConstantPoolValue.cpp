#include "XtensaConstantPoolValue.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

XtensaConstantPoolValue::XtensaConstantPoolValue(Type *Ty, XtensaCPKind Kind)
    : MachineConstantPoolValue(Ty), Kind(Kind) {}

XtensaConstantPoolValue::~XtensaConstantPoolValue() = default;

XtensaConstantPoolJumpTableAddr::XtensaConstantPoolJumpTableAddr(LLVMContext &C,
                                                                 int JTI)
    : XtensaConstantPoolValue(PointerType::get(C, 0),
                              XtensaCPKind::JumpTableAddr),
      JTI(JTI) {}

XtensaConstantPoolJumpTableAddr *
XtensaConstantPoolJumpTableAddr::create(LLVMContext &C, int JTI) {
  return new XtensaConstantPoolJumpTableAddr(C, JTI);
}

int XtensaConstantPoolJumpTableAddr::getExistingMachineCPValue(
    MachineConstantPool *CP, Align Alignment) {
  const std::vector<MachineConstantPoolEntry> &Constants = CP->getConstants();
  for (unsigned I = 0; I < Constants.size(); I++) {
    if (Constants[I].isMachineConstantPoolEntry() &&
        Constants[I].getAlign() >= Alignment) {
      auto *CPV =
          static_cast<XtensaConstantPoolValue *>(Constants[I].Val.MachineCPVal);
      if (XtensaConstantPoolJumpTableAddr *JumpTableCPV =
              dyn_cast<XtensaConstantPoolJumpTableAddr>(CPV)) {
        if (JumpTableCPV->JTI == JTI) {
          return I;
        }
      }
    }
  }

  return -1;
}

void XtensaConstantPoolJumpTableAddr::addSelectionDAGCSEId(
    FoldingSetNodeID &ID) {
  ID.AddInteger(JTI);
}

void XtensaConstantPoolJumpTableAddr::print(raw_ostream &O) const {
  O << printJumpTableEntryReference(JTI);
}
