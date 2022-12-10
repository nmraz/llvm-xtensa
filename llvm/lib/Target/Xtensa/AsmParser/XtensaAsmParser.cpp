#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

class XtensaAsmParser : public MCTargetAsmParser {
public:
  XtensaAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {}

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  bool ParseDirective(AsmToken DirectiveID) override;

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  void convertToMapAndConstraints(unsigned Kind,
                                  const OperandVector &Operands) override;
};

} // namespace

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaAsmParser() {
  RegisterMCAsmParser<XtensaAsmParser> X(getTheXtensaTarget());
}
