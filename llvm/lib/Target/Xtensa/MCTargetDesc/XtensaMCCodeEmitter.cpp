#include "XtensaMCCodeEmitter.h"
#include "XtensaBaseInfo.h"
#include "XtensaMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

void XtensaMCCodeEmitter::encodeInstruction(const MCInst &MI, raw_ostream &OS,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  unsigned Size = MCII.get(MI.getOpcode()).getSize();
  uint64_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);
  emitOpcode(Binary, Size, OS);
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

  if (MO.isImm()) {
    return MO.getImm();
  }

  assert(MO.isExpr());
  // TODO: record fixups here
  return 0;
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
XtensaMCCodeEmitter::getUImm5Sub32OpValue(const MCInst &MI, unsigned int OpIdx,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
  uint64_t Val = MI.getOperand(OpIdx).getImm();
  assert(Val < 32 && "Invalid uimm5s32 value");
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

MCCodeEmitter *llvm::createXtensaMCCodeEmitter(const MCInstrInfo &MCII,
                                               MCContext &Ctx) {
  return new XtensaMCCodeEmitter(MCII, Ctx);
}

#include "XtensaGenMCCodeEmitter.inc"
