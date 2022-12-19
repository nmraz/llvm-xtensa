#ifndef LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCCODEEMITTER_H
#define LLVM_LIB_TARGET_XTENSA_MCTARGETDESC_XTENSAMCCODEEMITTER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include <cstdint>

namespace llvm {

class MCContext;
class MCFixup;
class MCInst;
class MCInstrInfo;
class MCOperand;
class MCSubtargetInfo;
class raw_ostream;

class XtensaMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  XtensaMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), Ctx(Ctx) {}

  XtensaMCCodeEmitter(const XtensaMCCodeEmitter &) = delete;
  XtensaMCCodeEmitter &operator=(const XtensaMCCodeEmitter &) = delete;
  ~XtensaMCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  void emitOpcode(uint64_t Binary, unsigned int Size, raw_ostream &OS) const;

  // getBinaryCodeForInstr - TableGen'erated function for getting the
  // binary encoding for an instruction.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // getMachineOpValue - Return binary encoding of operand. If the machin
  // operand requires relocation, record the relocation and return zero.
  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  template <unsigned S>
  unsigned getShiftedImmOpValue(const MCInst &MI, unsigned OpIdx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  unsigned getUImm4Plus1OpValue(const MCInst &MI, unsigned OpIdx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  unsigned getUImm5Sub32OpValue(const MCInst &MI, unsigned OpIdx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  unsigned getB4ConstOpValue(const MCInst &MI, unsigned OpIdx,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getB4ConstUOpValue(const MCInst &MI, unsigned OpIdx,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const;

  unsigned getBrTarget8OpValue(const MCInst &MI, unsigned OpIdx,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;

  unsigned getBrTarget12OpValue(const MCInst &MI, unsigned OpIdx,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

  unsigned getL32RTarget16OpValue(const MCInst &MI, unsigned OpIdx,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;

  unsigned getJmpTarget18OpValue(const MCInst &MI, unsigned OpIdx,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getCallTarget18OpValue(const MCInst &MI, unsigned OpIdx,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;
};

} // end namespace llvm

#endif
