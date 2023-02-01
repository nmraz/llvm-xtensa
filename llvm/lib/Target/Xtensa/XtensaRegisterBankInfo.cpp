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
    if (MI.getOperand(Idx).isReg()) {
      LLT Ty = MRI.getType(MI.getOperand(Idx).getReg());
      auto Size = Ty.getSizeInBits();
      ValMappings[Idx] = &getValueMapping(0, Size, Xtensa::GPRRegBank);
    }
  }

  return getInstructionMapping(DefaultMappingID, 1,
                               getOperandsMapping(ValMappings), NumOperands);
}
