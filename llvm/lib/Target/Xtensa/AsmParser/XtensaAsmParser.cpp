#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmMacro.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>
#include <memory>

using namespace llvm;

namespace {

class XtensaAsmParser : public MCTargetAsmParser {
private:
#define GET_ASSEMBLER_HEADER
#include "XtensaGenAsmMatcher.inc"

public:
  XtensaAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {}

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;

  bool parseOperand(OperandVector &Operands);

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  bool ParseDirective(AsmToken DirectiveID) override;

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
};

class XtensaOperand : public MCParsedAsmOperand {
private:
  enum KindTy {
    k_Token,
    k_Register,
    k_Immediate,
  } Kind;

  SMLoc StartLoc, EndLoc;

  struct TokOp {
    const char *Data;
    unsigned Length;
  };

  struct RegOp {
    unsigned RegNo;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  union {
    struct TokOp Tok;
    struct RegOp Reg;
    struct ImmOp Imm;
  };

public:
  XtensaOperand(KindTy K, SMLoc S, SMLoc E) : Kind(K), StartLoc(S), EndLoc(E) {}

  static std::unique_ptr<XtensaOperand> createToken(StringRef Token, SMLoc S) {
    auto Operand = std::make_unique<XtensaOperand>(k_Token, S, S);
    Operand->Tok.Data = Token.data();
    Operand->Tok.Length = Token.size();
    return Operand;
  }

  static std::unique_ptr<XtensaOperand> createReg(unsigned RegNo, SMLoc S,
                                                  SMLoc E) {
    auto Operand = std::make_unique<XtensaOperand>(k_Register, S, E);
    Operand->Reg.RegNo = RegNo;
    return Operand;
  }

  static std::unique_ptr<XtensaOperand> createImm(const MCExpr *Val, SMLoc S,
                                                  SMLoc E) {
    auto Operand = std::make_unique<XtensaOperand>(k_Immediate, S, E);
    Operand->Imm.Val = Val;
    return Operand;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  bool isToken() const override { return Kind == k_Token; }
  bool isImm() const override { return Kind == k_Immediate; }
  bool isReg() const override { return Kind == k_Register; }
  bool isMem() const override { return false; }

  unsigned getReg() const override {
    assert(isReg() && "Invalid access!");
    return Reg.RegNo;
  }

  StringRef getToken() const {
    assert(isToken() && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  void print(raw_ostream &OS) const override;

  void addRegOperands(MCInst &Inst, unsigned N) const {}
  void addImmOperands(MCInst &Inst, unsigned N) const {}
};

void XtensaOperand::print(raw_ostream &OS) const {
  switch (Kind) {
  case k_Token:
    OS << getToken();
    break;

  case k_Register:
    break;

  case k_Immediate:
    break;
  }
}

} // namespace

// Auto-generated implementation below
static unsigned MatchRegisterName(StringRef Name);

bool XtensaAsmParser::ParseRegister(unsigned int &RegNo, SMLoc &StartLoc,
                                    SMLoc &EndLoc) {
  if (tryParseRegister(RegNo, StartLoc, EndLoc) != MatchOperand_Success) {
    Error(getTok().getLoc(), "expected a register name");
    return true;
  }

  return false;
}

OperandMatchResultTy XtensaAsmParser::tryParseRegister(unsigned int &RegNo,
                                                       SMLoc &StartLoc,
                                                       SMLoc &EndLoc) {
  if (!getLexer().is(AsmToken::Identifier)) {
    return MatchOperand_NoMatch;
  }

  auto NameTok = getParser().getTok();
  StringRef Name = NameTok.getString();

  StartLoc = NameTok.getLoc();
  EndLoc = NameTok.getEndLoc();

  unsigned MatchedRegNo = MatchRegisterName(Name);
  if (MatchedRegNo) {
    getParser().Lex();
    RegNo = MatchedRegNo;
    return MatchOperand_Success;
  }

  return MatchOperand_NoMatch;
}

bool XtensaAsmParser::parseOperand(OperandVector &Operands) {
  SMLoc StartLoc;
  SMLoc EndLoc;
  unsigned RegNo;

  OperandMatchResultTy RegResult = tryParseRegister(RegNo, StartLoc, EndLoc);

  switch (RegResult) {
  case MatchOperand_Success:
    Operands.push_back(XtensaOperand::createReg(RegNo, StartLoc, EndLoc));
    return false;
  case MatchOperand_NoMatch:
    break;
  case MatchOperand_ParseFail:
    return true;
  }

  StartLoc = getLexer().getLoc();

  const MCExpr *ImmVal;
  if (getParser().parseExpression(ImmVal, EndLoc)) {
    // `parseExpression` will print the error itself.
    return true;
  }

  Operands.push_back(XtensaOperand::createImm(ImmVal, StartLoc, EndLoc));

  return false;
}

bool XtensaAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                       StringRef Name, SMLoc NameLoc,
                                       OperandVector &Operands) {
  Operands.push_back(XtensaOperand::createToken(Name, NameLoc));

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    if (parseOperand(Operands)) {
      return true;
    }

    while (parseOptionalToken(AsmToken::Comma)) {
      if (parseOperand(Operands)) {
        return true;
      }
    }
  }

  return false;
}

bool XtensaAsmParser::ParseDirective(AsmToken DirectiveID) {
  // We have no custom directives for now
  return true;
}

bool XtensaAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                                              OperandVector &Operands,
                                              MCStreamer &Out,
                                              uint64_t &ErrorInfo,
                                              bool MatchingInlineAsm) {
  return true;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaAsmParser() {
  RegisterMCAsmParser<XtensaAsmParser> X(getTheXtensaTarget());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "XtensaGenAsmMatcher.inc"
