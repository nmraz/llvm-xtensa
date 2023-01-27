#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAREGISTERBANKINFO_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAREGISTERBANKINFO_H

#include "llvm/CodeGen/RegisterBankInfo.h"

#define GET_REGBANK_DECLARATIONS
#include "XtensaGenRegisterBank.inc"

namespace llvm {

class MachineInstr;
class TargetRegisterInfo;

class XtensaGenRegisterBankInfo : public RegisterBankInfo {
#define GET_TARGET_REGBANK_CLASS
#include "XtensaGenRegisterBank.inc"
};

/// This class provides the information for the target register banks.
class XtensaRegisterBankInfo final : public XtensaGenRegisterBankInfo {
public:
  XtensaRegisterBankInfo(const TargetRegisterInfo &TRI);

  const RegisterBank &getRegBankFromRegClass(const TargetRegisterClass &RC,
                                             LLT) const override;

  const InstructionMapping &
  getInstrMapping(const MachineInstr &MI) const override;
};
} // namespace llvm

#endif
