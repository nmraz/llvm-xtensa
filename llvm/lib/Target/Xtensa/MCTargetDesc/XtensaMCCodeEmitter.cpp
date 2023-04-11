#include "XtensaMCCodeEmitter.h"
#include "XtensaBaseInfo.h"
#include "XtensaFixupKinds.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

static MCFixup createExprFixup(const MCExpr *Expr, unsigned Kind) {
  return MCFixup::create(0, Expr, (MCFixupKind)Kind, Expr->getLoc());
}

void XtensaMCCodeEmitter::encodeInstruction(const MCInst &MI, raw_ostream &OS,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  unsigned Size = MCII.get(MI.getOpcode()).getSize();
  if (tryEmitRelaxedBranch(MI, OS, Fixups, STI)) {
    return;
  }
  uint64_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);
  emitOpcode(Binary, Size, OS);
}

static Optional<unsigned> getInvertedUnrelaxedOpcode(unsigned Opcode) {
  switch (Opcode) {
  case Xtensa::BALLRelaxed:
    return Xtensa::BNALL;
  case Xtensa::BANYRelaxed:
    return Xtensa::BNONE;
  case Xtensa::BBCRelaxed:
    return Xtensa::BBS;
  case Xtensa::BBCIRelaxed:
    return Xtensa::BBSI;
  case Xtensa::BBSRelaxed:
    return Xtensa::BBC;
  case Xtensa::BBSIRelaxed:
    return Xtensa::BBCI;
  case Xtensa::BEQRelaxed:
    return Xtensa::BNE;
  case Xtensa::BEQIRelaxed:
    return Xtensa::BNEI;
  case Xtensa::BEQZRelaxed:
    return Xtensa::BNEZ;
  case Xtensa::BGERelaxed:
    return Xtensa::BLT;
  case Xtensa::BGEIRelaxed:
    return Xtensa::BLTI;
  case Xtensa::BGEURelaxed:
    return Xtensa::BLTU;
  case Xtensa::BGEUIRelaxed:
    return Xtensa::BLTUI;
  case Xtensa::BGEZRelaxed:
    return Xtensa::BLTZ;
  case Xtensa::BLTRelaxed:
    return Xtensa::BGE;
  case Xtensa::BLTIRelaxed:
    return Xtensa::BGEI;
  case Xtensa::BLTURelaxed:
    return Xtensa::BGEU;
  case Xtensa::BLTUIRelaxed:
    return Xtensa::BGEUI;
  case Xtensa::BLTZRelaxed:
    return Xtensa::BGEZ;
  case Xtensa::BNALLRelaxed:
    return Xtensa::BALL;
  case Xtensa::BNERelaxed:
    return Xtensa::BEQ;
  case Xtensa::BNEIRelaxed:
    return Xtensa::BEQI;
  case Xtensa::BNEZRelaxed:
    return Xtensa::BEQZ;
  case Xtensa::BNONERelaxed:
    return Xtensa::BANY;
  default:
    return None;
  }
}

bool XtensaMCCodeEmitter::tryEmitRelaxedBranch(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  unsigned Opcode = MI.getOpcode();
  Optional<unsigned> MaybeInvertedOpcode = getInvertedUnrelaxedOpcode(Opcode);
  if (!MaybeInvertedOpcode) {
    return false;
  }
  unsigned InvertedOpcode = *MaybeInvertedOpcode;

  const MCInstrDesc &OrigDesc = MCII.get(Opcode);
  const MCInstrDesc &Desc = MCII.get(InvertedOpcode);
  assert(OrigDesc.getNumOperands() == Desc.getNumOperands());

  // Assumption: the branch target is always the last operand.
  unsigned NumOperands = Desc.getNumOperands();
  MCInstBuilder InvertedInst(InvertedOpcode);
  for (unsigned I = 0; I < NumOperands - 1; I++) {
    InvertedInst.addOperand(MI.getOperand(I));
  }

  // Note: this must be signed for the offset computation below to be correct.
  int InvertedInstSize = Desc.getSize();

  // For now, all relaxable branches end up using an offset relative to `pc+4`.
  InvertedInst.addImm(InvertedInstSize - 4);
  emitOpcode(getBinaryCodeForInstr(InvertedInst, Fixups, STI), InvertedInstSize,
             OS);

  MCInst Jump = MCInstBuilder(Xtensa::J).addOperand(
      MI.getOperand(Desc.getNumOperands() - 1));
  emitOpcode(getBinaryCodeForInstr(Jump, Fixups, STI), 3, OS);

  return true;
}

void XtensaMCCodeEmitter::emitOpcode(uint64_t Value, unsigned int Size,
                                     raw_ostream &OS) const {
  // Emit as little-endian
  for (unsigned i = 0; i < Size; i++) {
    OS << static_cast<uint8_t>(Value);
    Value >>= 8;
  }
}

unsigned
XtensaMCCodeEmitter::getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  }

  assert(MO.isImm() && "Unsupported custom operand");

  return MO.getImm();
}

template <unsigned S>
unsigned
XtensaMCCodeEmitter::getShiftedImmOpValue(const MCInst &MI, unsigned OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  return Val >> S;
}

unsigned
XtensaMCCodeEmitter::getUImm4Plus1OpValue(const MCInst &MI, unsigned int OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  assert(Val >= 1 && Val <= 16 && "Invalid uimm4p1 value");
  return Val - 1;
}

unsigned
XtensaMCCodeEmitter::getUImm4Plus7OpValue(const MCInst &MI, unsigned int OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  assert(Val >= 7 && Val <= 22 && "Invalid uimm4p7 value");
  return Val - 7;
}

unsigned
XtensaMCCodeEmitter::getAddiNImm4OpValue(const MCInst &MI, unsigned OpIdx,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  int64_t Val = MI.getOperand(OpIdx).getImm();
  Optional<uint32_t> Encoded = XtensaII::encodeAddiNImm4(Val);
  assert(Encoded.has_value() && "Invalid addin_imm4 value");
  return *Encoded;
}

unsigned
XtensaMCCodeEmitter::getUImm5Sub32OpValue(const MCInst &MI, unsigned int OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  assert(Val >= 1 && Val <= 31 && "Invalid uimm5s32 value");
  return 32 - Val;
}

unsigned
XtensaMCCodeEmitter::getB4ConstOpValue(const MCInst &MI, unsigned int OpIdx,
                                       SmallVectorImpl<MCFixup> &Fixups,
                                       const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  Optional<uint64_t> Encoded = XtensaII::encodeB4Const(Val);
  assert(Encoded.has_value() && "Invalid b4const value");
  return *Encoded;
}

unsigned
XtensaMCCodeEmitter::getB4ConstUOpValue(const MCInst &MI, unsigned int OpIdx,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  Optional<uint64_t> Encoded = XtensaII::encodeB4ConstU(Val);
  assert(Encoded.has_value() && "Invalid b4constu value");
  return *Encoded;
}

unsigned
XtensaMCCodeEmitter::getBrTarget8OpValue(const MCInst &MI, unsigned int OpIdx,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  if (MO.isImm()) {
    return getMachineOpValue(MI, MO, Fixups, STI);
  }

  Fixups.push_back(
      createExprFixup(MO.getExpr(), Xtensa::fixup_xtensa_brtarget8));
  return 0;
}

unsigned
XtensaMCCodeEmitter::getBrTarget12OpValue(const MCInst &MI, unsigned int OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  if (MO.isImm()) {
    return getMachineOpValue(MI, MO, Fixups, STI);
  }

  Fixups.push_back(
      createExprFixup(MO.getExpr(), Xtensa::fixup_xtensa_brtarget12));
  return 0;
}

unsigned XtensaMCCodeEmitter::getL32RTarget16OpValue(
    const MCInst &MI, unsigned int OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  if (MO.isImm()) {
    return static_cast<int32_t>(getMachineOpValue(MI, MO, Fixups, STI)) / 4;
  }

  Fixups.push_back(
      createExprFixup(MO.getExpr(), Xtensa::fixup_xtensa_l32rtarget16));
  return 0;
}

unsigned
XtensaMCCodeEmitter::getJmpTarget18OpValue(const MCInst &MI, unsigned int OpIdx,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  if (MO.isImm()) {
    return getMachineOpValue(MI, MO, Fixups, STI);
  }

  Fixups.push_back(
      createExprFixup(MO.getExpr(), Xtensa::fixup_xtensa_jmptarget18));
  return 0;
}

unsigned XtensaMCCodeEmitter::getCallTarget18OpValue(
    const MCInst &MI, unsigned int OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  if (MO.isImm()) {
    return static_cast<int32_t>(getMachineOpValue(MI, MO, Fixups, STI)) / 4;
  }

  Fixups.push_back(
      createExprFixup(MO.getExpr(), Xtensa::fixup_xtensa_calltarget18));
  return 0;
}

MCCodeEmitter *llvm::createXtensaMCCodeEmitter(const MCInstrInfo &MCII,
                                               MCContext &Ctx) {
  return new XtensaMCCodeEmitter(MCII, Ctx);
}

#include "XtensaGenMCCodeEmitter.inc"
