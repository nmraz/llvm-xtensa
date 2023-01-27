#include "XtensaRegisterBankInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "XtensaRegisterInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/RegisterBank.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace llvm;

#define GET_TARGET_REGBANK_IMPL
#include "XtensaGenRegisterBank.inc"

namespace llvm {
namespace Xtensa {

RegisterBankInfo::PartialMapping PartMappings[] = {
    {0, 32, GPRRegBank},
};

enum ValueMappingIdx {
  InvalidIdx = 0,
  GPRIdx = 1,
};

RegisterBankInfo::ValueMapping ValueMappings[] = {
    // Invalid mapping
    {nullptr, 0},
    // Up to 3 operands in GPRs
    {&PartMappings[0], 1},
    {&PartMappings[0], 1},
    {&PartMappings[0], 1},
};

} // namespace Xtensa
} // namespace llvm

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

  int NumOperands = MI.getNumOperands();
  assert(NumOperands <= 3 && "Too many operands in instruction!");

  // Up to 3 operands in GPRs
  RegisterBankInfo::ValueMapping *OperandsMapping = &Xtensa::ValueMappings[1];
  return getInstructionMapping(DefaultMappingID, 1, OperandsMapping,
                               NumOperands);
}
