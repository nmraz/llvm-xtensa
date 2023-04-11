#include "MCTargetDesc/XtensaBaseInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/ADT/Optional.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "xtensa-disassembler"

using namespace llvm;
using DecodeStatus = MCDisassembler::DecodeStatus;

namespace {

class XtensaDisassembler : public MCDisassembler {
public:
  XtensaDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};

} // namespace

static bool tryAddingSymbolicOperand(const MCDisassembler *Decoder,
                                     MCInst &Inst, int64_t Value,
                                     uint64_t Address, bool IsBranch) {
  return Decoder->tryAddingSymbolicOperand(Inst, Value, Address, IsBranch, 0, 0,
                                           3);
}

static DecodeStatus DecodeGPRRegisterClass(MCInst &MI, uint32_t Imm,
                                           uint64_t Address,
                                           const MCDisassembler *Decoder) {
  unsigned Reg =
      XtensaMCRegisterClasses[Xtensa::GPRRegClassID].getRegister(Imm);
  MI.addOperand(MCOperand::createReg(Reg));
  return DecodeStatus::Success;
}

template <unsigned S>
static DecodeStatus decodeShiftedUImm(MCInst &MI, uint32_t Imm,
                                      uint64_t Address,
                                      const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(Imm << S));
  return DecodeStatus::Success;
}

template <unsigned N, unsigned S>
static DecodeStatus decodeShiftedSImm(MCInst &MI, uint32_t Imm,
                                      uint64_t Address,
                                      const MCDisassembler *Decoder) {
  // Note: this must be unsigned before the subsequent shift as we will be
  // shifting through bit 31, which is UB on signed values.
  uint32_t Extended = SignExtend32<N>(Imm);
  MI.addOperand(MCOperand::createImm(static_cast<int32_t>(Extended << S)));
  return DecodeStatus::Success;
}

static DecodeStatus decodeB4Const(MCInst &MI, uint32_t Imm, uint64_t Address,
                                  const MCDisassembler *Decoder) {
  Optional<int32_t> Decoded = XtensaII::decodeB4Const(Imm);
  assert(Decoded && "Unexpected b4const immediate");
  MI.addOperand(MCOperand::createImm(*Decoded));
  return DecodeStatus::Success;
}

static DecodeStatus decodeB4ConstU(MCInst &MI, uint32_t Imm, uint64_t Address,
                                   const MCDisassembler *Decoder) {
  Optional<uint32_t> Decoded = XtensaII::decodeB4ConstU(Imm);
  assert(Decoded && "Invalid b4constu immediate");
  MI.addOperand(MCOperand::createImm(*Decoded));
  return DecodeStatus::Success;
}

static DecodeStatus decodeUImm4Plus1(MCInst &MI, uint32_t Imm, uint64_t Address,
                                     const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(Imm + 1));
  return DecodeStatus::Success;
}

static DecodeStatus decodeUImm4Plus7(MCInst &MI, uint32_t Imm, uint64_t Address,
                                     const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(Imm + 7));
  return DecodeStatus::Success;
}

static DecodeStatus decodeAddiNImm4(MCInst &MI, uint32_t Imm, uint64_t Address,
                                    const MCDisassembler *Decoder) {
  Optional<int32_t> Decoded = XtensaII::decodeAddiNImm4(Imm);
  assert(Decoded && "Invalid addin_imm4 immediate");
  MI.addOperand(MCOperand::createImm(*Decoded));
  return DecodeStatus::Success;
}

static DecodeStatus decodeUImm5Sub32(MCInst &MI, uint32_t Imm, uint64_t Address,
                                     const MCDisassembler *Decoder) {
  MI.addOperand(MCOperand::createImm(32 - Imm));
  return DecodeStatus::Success;
}

static DecodeStatus decodeMoviNImm7(MCInst &MI, uint32_t Imm, uint64_t Address,
                                    const MCDisassembler *Decoder) {
  // Extend from 7 to 32 bits using the AND of bits 5 and 6 in the immediate.
  int32_t Extension = SignExtend32<7>(Imm & (Imm << 1) & (1 << 6));
  MI.addOperand(MCOperand::createImm(static_cast<int32_t>(Imm | Extension)));
  return DecodeStatus::Success;
}

template <unsigned N>
static DecodeStatus decodeBrTarget(MCInst &MI, uint32_t Imm, uint64_t Address,
                                   const MCDisassembler *Decoder) {
  int32_t Value = SignExtend32<N>(Imm);
  if (!tryAddingSymbolicOperand(Decoder, MI,
                                Xtensa_MC::evaluateBranchTarget(Address, Value),
                                Address, true)) {
    MI.addOperand(MCOperand::createImm(Value));
  }
  return DecodeStatus::Success;
}

static DecodeStatus decodeL32RTarget16(MCInst &MI, uint32_t Imm,
                                       uint64_t Address,
                                       const MCDisassembler *Decoder) {
  int32_t Value = (0xffff0000 | Imm) << 2;
  if (!tryAddingSymbolicOperand(Decoder, MI,
                                Xtensa_MC::evaluateL32RTarget(Address, Value),
                                Address, false)) {
    MI.addOperand(MCOperand::createImm(Value));
  }
  return DecodeStatus::Success;
}

static DecodeStatus decodeCallTarget18(MCInst &MI, uint32_t Imm,
                                       uint64_t Address,
                                       const MCDisassembler *Decoder) {
  int32_t Value = Imm << 2;
  if (!tryAddingSymbolicOperand(Decoder, MI,
                                Xtensa_MC::evaluateCallTarget(Address, Value),
                                Address, false)) {
    MI.addOperand(MCOperand::createImm(Value));
  }
  return DecodeStatus::Success;
}

#include "XtensaGenDisassemblerTables.inc"

DecodeStatus XtensaDisassembler::getInstruction(MCInst &Instr, uint64_t &Size,
                                                ArrayRef<uint8_t> Bytes,
                                                uint64_t Address,
                                                raw_ostream &CStream) const {
  Size = 0;

  if (Bytes.size() < 2) {
    return DecodeStatus::Fail;
  }

  uint16_t NarrowInst = (Bytes[0] << 0) | (Bytes[1] << 8);

  DecodeStatus Status = decodeInstruction(DecoderTable16, Instr, NarrowInst,
                                          Address, this, getSubtargetInfo());
  if (Status != DecodeStatus::Fail) {
    Size = 2;
    return Status;
  }

  if (Bytes.size() < 3) {
    return DecodeStatus::Fail;
  }

  uint32_t WideInst = (Bytes[0] << 0) | (Bytes[1] << 8) | (Bytes[2] << 16);
  Status = decodeInstruction(DecoderTable24, Instr, WideInst, Address, this,
                             getSubtargetInfo());
  if (Status != DecodeStatus::Fail) {
    Size = 3;
  }
  return Status;
}

static MCDisassembler *createXtensaDisassembler(const Target &T,
                                                const MCSubtargetInfo &STI,
                                                MCContext &Ctx) {
  return new XtensaDisassembler(STI, Ctx);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheXtensaTarget(),
                                         createXtensaDisassembler);
}
