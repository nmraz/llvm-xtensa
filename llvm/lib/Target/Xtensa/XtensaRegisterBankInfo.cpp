#include "XtensaRegisterBankInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaRegisterInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterBank.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LowLevelTypeImpl.h"
#include <cassert>

using namespace llvm;

#define GET_TARGET_REGBANK_IMPL
#include "XtensaGenRegisterBank.inc"

XtensaRegisterBankInfo::XtensaRegisterBankInfo(const TargetRegisterInfo &TRI) {}

const RegisterBank &
XtensaRegisterBankInfo::getRegBankFromRegClass(const TargetRegisterClass &RC,
                                               LLT) const {
  switch (RC.getID()) {
  case Xtensa::GPRRegClassID:
    return getRegBank(Xtensa::GPRRegBankID);
  case Xtensa::SARCRegClassID:
    return getRegBank(Xtensa::SARRegBankID);
  default:
    llvm_unreachable("Register class not supported");
  }
}

const RegisterBankInfo::InstructionMapping &
XtensaRegisterBankInfo::getInstrMapping(const MachineInstr &MI) const {
  const auto &Mapping = getInstrMappingImpl(MI);
  if (Mapping.isValid()) {
    return Mapping;
  }

  const MachineFunction &MF = *MI.getParent()->getParent();
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  unsigned NumOperands = MI.getNumOperands();

  SmallVector<const ValueMapping *, 4> ValMappings(NumOperands);
  for (unsigned Idx = 0; Idx < NumOperands; Idx++) {
    auto MO = MI.getOperand(Idx);
    if (!MO.isReg()) {
      continue;
    }

    Register Reg = MO.getReg();
    if (!Reg.isPhysical()) {
      LLT Ty = MRI.getType(Reg);
      assert((Ty.isScalar() || Ty.isPointer()) && Ty.getSizeInBits() == 32);
    }

    bool IsSAR = Reg.id() == Xtensa::SAR;
    unsigned Length = IsSAR ? 8 : 32;

    ValMappings[Idx] = &getValueMapping(
        0, Length, IsSAR ? Xtensa::SARRegBank : Xtensa::GPRRegBank);
  }

  return getInstructionMapping(DefaultMappingID, 1,
                               getOperandsMapping(ValMappings), NumOperands);
}
