#ifndef LLVM_LIB_TARGET_XTENSA_XTENSASUBTARGET_H
#define LLVM_LIB_TARGET_XTENSA_XTENSASUBTARGET_H

#include "XtensaFrameLowering.h"
#include "XtensaISelLowering.h"
#include "XtensaInstrInfo.h"
#include "XtensaRegisterInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/RegisterBankInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include <memory>

#define GET_SUBTARGETINFO_HEADER
#include "XtensaGenSubtargetInfo.inc"

namespace llvm {

class XtensaTargetMachine;

class XtensaSubtarget : public XtensaGenSubtargetInfo {
private:
  XtensaFrameLowering FrameLowering;
  XtensaInstrInfo InstrInfo;
  XtensaTargetLowering TLInfo;

  std::unique_ptr<CallLowering> CallLoweringInfo;
  std::unique_ptr<RegisterBankInfo> RegBankInfo;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<InstructionSelector> InstSelector;

  bool HasSalt;
  bool HasLX7;

public:
  XtensaSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                  const XtensaTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  XtensaSubtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);

  const XtensaInstrInfo *getInstrInfo() const override { return &InstrInfo; }

  const XtensaRegisterInfo *getRegisterInfo() const override {
    return &getInstrInfo()->getRegisterInfo();
  }

  const XtensaFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const TargetLowering *getTargetLowering() const override { return &TLInfo; }

  bool hasSalt() const { return HasSalt; }

  // GlobalISel

  const CallLowering *getCallLowering() const override;
  const RegisterBankInfo *getRegBankInfo() const override;
  const LegalizerInfo *getLegalizerInfo() const override;
  InstructionSelector *getInstructionSelector() const override;
};

} // namespace llvm

#endif
