#include "MCTargetDesc/XtensaBaseInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "Xtensa.h"
#include "XtensaInstrInfo.h"
#include "XtensaSubtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/MathExtras.h"
#include <cstdint>

#define DEBUG_TYPE "xtensa-size-reduction"

using namespace llvm;

class XtensaSizeReduction : public MachineFunctionPass {
public:
  static char ID;

  XtensaSizeReduction();
  StringRef getPassName() const override { return "Xtensa Size Reduction"; }

  bool runOnMachineFunction(MachineFunction &MF) override;
};

XtensaSizeReduction::XtensaSizeReduction() : MachineFunctionPass(ID) {
  initializeXtensaSizeReductionPass(*PassRegistry::getPassRegistry());
}

static Optional<unsigned> getNarrowOpcodeFor(const MachineInstr &MI) {
  unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
  case Xtensa::MOVI:
    if (XtensaII::isMoviNImm7(MI.getOperand(1).getImm())) {
      return Xtensa::MOVIN;
    }
    break;
  case Xtensa::ADDI:
    if (XtensaII::encodeAddiNImm4(MI.getOperand(2).getImm()).has_value()) {
      return Xtensa::ADDIN;
    }
    break;
  case Xtensa::L32I:
  case Xtensa::S32I:
    if (isShiftedUInt<4, 2>(MI.getOperand(2).getImm())) {
      return Opcode == Xtensa::L32I ? Xtensa::L32IN : Xtensa::S32IN;
    }
    break;
  }

  return None;
}

bool XtensaSizeReduction::runOnMachineFunction(MachineFunction &MF) {
  const XtensaInstrInfo &TII =
      *MF.getSubtarget<XtensaSubtarget>().getInstrInfo();
  bool Modified = false;

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (auto NarrowOpcode = getNarrowOpcodeFor(MI)) {
        MI.setDesc(TII.get(*NarrowOpcode));
      }
    }
  }

  return Modified;
}

char XtensaSizeReduction::ID = 0;
INITIALIZE_PASS_BEGIN(
    XtensaSizeReduction, DEBUG_TYPE,
    "Convert Xtensa instructions to smaller forms where possible", false, false)
INITIALIZE_PASS_END(
    XtensaSizeReduction, DEBUG_TYPE,
    "Convert Xtensa instructions to smaller forms where possible", false, false)

namespace llvm {
FunctionPass *createXtensaSizeReduction() { return new XtensaSizeReduction(); }
} // end namespace llvm
