#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAMCTARGETDESC_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAMCTARGETDESC_H

#include "llvm/MC/MCObjectWriter.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;

MCCodeEmitter *createXtensaMCCodeEmitter(const MCInstrInfo &MCII,
                                         MCContext &Ctx);

MCAsmBackend *createXtensaAsmBackend(const Target &T,
                                     const MCSubtargetInfo &STI,
                                     const MCRegisterInfo &MRI,
                                     const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter>
createXtensaELFObjectWriter(const Triple &TT);

} // namespace llvm

// Defines symbolic names for Xtensa registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "XtensaGenRegisterInfo.inc"

// Defines symbolic names for the Xtensa instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "XtensaGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "XtensaGenSubtargetInfo.inc"

#endif
