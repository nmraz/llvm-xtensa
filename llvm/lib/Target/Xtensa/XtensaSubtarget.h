#ifndef LLVM_LIB_TARGET_XTENSA_XTENSASUBTARGET_H
#define LLVM_LIB_TARGET_XTENSA_XTENSASUBTARGET_H

#include "XtensaFrameLowering.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "XtensaGenSubtargetInfo.inc"

namespace llvm {

class XtensaTargetMachine;

class XtensaSubtarget : public XtensaGenSubtargetInfo {
private:
  XtensaFrameLowering FrameLowering;

public:
  XtensaSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                  const XtensaTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const TargetInstrInfo *getInstrInfo() const override { return nullptr; }

  const TargetFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const CallLowering *getCallLowering() const override { return nullptr; }

  const TargetRegisterInfo *getRegisterInfo() const override { return nullptr; }

  const RegisterBankInfo *getRegBankInfo() const override { return nullptr; }

  const LegalizerInfo *getLegalizerInfo() const override { return nullptr; }

  InstructionSelector *getInstructionSelector() const override {
    return nullptr;
  }
};

} // namespace llvm

#endif
