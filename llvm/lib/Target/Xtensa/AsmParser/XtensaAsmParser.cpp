#include "MCTargetDesc/XtensaBaseInfo.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmMacro.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

using namespace llvm;

namespace {

class XtensaAsmParser : public MCTargetAsmParser {
private:
#define GET_ASSEMBLER_HEADER
#include "XtensaGenAsmMatcher.inc"

public:
  enum XtensaMatchResultTy {
    Match_XtensaFirst = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "XtensaGenAsmMatcher.inc"
  };

  XtensaAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {}

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;

  OperandMatchResultTy parseExprImm(OperandVector &Operands);
  OperandMatchResultTy parseConstantImm(OperandVector &Operands);

  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);

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

  StringRef getToken() const {
    assert(isToken() && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  const MCExpr *getImm() const {
    assert(isImm() && "Invalid access!");
    return Imm.Val;
  }

  uint64_t getConstImm() const {
    return cast<MCConstantExpr>(getImm())->getValue();
  }

  unsigned getReg() const override {
    assert(isReg() && "Invalid access!");
    return Reg.RegNo;
  }

  template <size_t Bits, size_t Shift> bool isShiftedUImm() const {
    return isShiftedUInt<Bits, Shift>(getConstImm());
  }

  template <size_t Bits, size_t Shift> bool isShiftedSImm() const {
    return isShiftedInt<Bits, Shift>(getConstImm());
  }

  bool isUImm4Plus1() const;
  bool isUImm4Plus7() const;
  bool isMoviNImm7() const;
  bool isB4Const() const;
  bool isB4ConstU() const;

  void print(raw_ostream &OS) const override;

  void addRegOperands(MCInst &Inst, unsigned N) const;
  void addImmOperands(MCInst &Inst, unsigned N) const;
};

bool XtensaOperand::isUImm4Plus1() const {
  uint64_t Val = getConstImm();
  // Note: this behaves correctly even in the face of underflow
  return isUInt<4>(Val - 1);
}

bool XtensaOperand::isUImm4Plus7() const {
  uint64_t Val = getConstImm();
  return Val >= 7 && Val <= 22;
}

bool XtensaOperand::isMoviNImm7() const {
  int64_t Val = static_cast<int64_t>(getConstImm());
  return XtensaII::isMoviNImm7(Val);
}

bool XtensaOperand::isB4Const() const {
  return XtensaII::encodeB4Const(getConstImm()).has_value();
}

bool XtensaOperand::isB4ConstU() const {
  return XtensaII::encodeB4ConstU(getConstImm()).has_value();
}

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

void XtensaOperand::addRegOperands(MCInst &Inst, unsigned int N) const {
  assert(N == 1 && "Invalid number of operands");
  Inst.addOperand(MCOperand::createReg(getReg()));
}

void XtensaOperand::addImmOperands(MCInst &Inst, unsigned int N) const {
  assert(N == 1 && "Invalid number of operands");
  const MCExpr *Expr = getImm();
  if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr)) {
    Inst.addOperand(MCOperand::createImm(CE->getValue()));
  } else {
    Inst.addOperand(MCOperand::createExpr(getImm()));
  }
}

} // namespace

// Auto-generated implementation below
static unsigned MatchRegisterName(StringRef Name);
static unsigned MatchRegisterAltName(StringRef Name);
static std::string XtensaMnemonicSpellCheck(StringRef S,
                                            const FeatureBitset &FBS,
                                            unsigned VariantID = 0);
static const char *
getMatchKindDiag(XtensaAsmParser::XtensaMatchResultTy MatchResult);

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
  if (!MatchedRegNo) {
    MatchedRegNo = MatchRegisterAltName(Name);
  }

  if (MatchedRegNo) {
    getParser().Lex();
    RegNo = MatchedRegNo;
    return MatchOperand_Success;
  }

  return MatchOperand_NoMatch;
}

OperandMatchResultTy XtensaAsmParser::parseExprImm(OperandVector &Operands) {
  SMLoc StartLoc = getTok().getLoc();
  SMLoc EndLoc;
  const MCExpr *ImmVal;

  if (getParser().parseExpression(ImmVal, EndLoc)) {
    return MatchOperand_ParseFail;
  }

  Operands.push_back(XtensaOperand::createImm(ImmVal, StartLoc, EndLoc));
  return MatchOperand_Success;
}

OperandMatchResultTy
XtensaAsmParser::parseConstantImm(OperandVector &Operands) {
  SMLoc StartLoc = getTok().getLoc();
  SMLoc EndLoc;
  const MCExpr *ImmVal;

  if (getParser().parseExpression(ImmVal, EndLoc)) {
    return MatchOperand_ParseFail;
  }

  if (!isa<MCConstantExpr>(ImmVal)) {
    Error(StartLoc, "operand must be a constant", SMRange(StartLoc, EndLoc));
    return MatchOperand_ParseFail;
  }

  Operands.push_back(XtensaOperand::createImm(ImmVal, StartLoc, EndLoc));
  return MatchOperand_Success;
}

bool XtensaAsmParser::parseOperand(OperandVector &Operands,
                                   StringRef Mnemonic) {
  SMLoc StartLoc;
  SMLoc EndLoc;

  // Our custom parsers handle everything but registers
  OperandMatchResultTy Result = MatchOperandParserImpl(Operands, Mnemonic);
  if (Result == MatchOperand_Success) {
    return false;
  }

  if (Result == MatchOperand_ParseFail) {
    return true;
  }

  unsigned RegNo;
  OperandMatchResultTy RegResult = tryParseRegister(RegNo, StartLoc, EndLoc);

  if (RegResult == MatchOperand_Success) {
    Operands.push_back(XtensaOperand::createReg(RegNo, StartLoc, EndLoc));
    return false;
  }

  if (RegResult == MatchOperand_ParseFail) {
    // We've already emitted an error, just propagate it.
    return true;
  }

  // Try to parse a generic expression. In practice, this case should not be hit
  // for any recognized instructions, but it should let us get far enough
  // through the parsing to report invalid instructions later.
  return parseExprImm(Operands) != MatchOperand_Success;
}

bool XtensaAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                       StringRef Name, SMLoc NameLoc,
                                       OperandVector &Operands) {
  if (Name.consume_front("_")) {
    // TODO: force wide/narrow instruction
  }

  Operands.push_back(XtensaOperand::createToken(Name, NameLoc));

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    if (parseOperand(Operands, Name)) {
      return true;
    }

    while (parseOptionalToken(AsmToken::Comma)) {
      if (parseOperand(Operands, Name)) {
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

static SMLoc refineErrorLoc(const SMLoc Loc, const OperandVector &Operands,
                            uint64_t ErrorInfo) {
  if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size()) {
    SMLoc ErrorLoc = Operands[ErrorInfo]->getStartLoc();
    if (ErrorLoc == SMLoc())
      return Loc;
    return ErrorLoc;
  }
  return Loc;
}

bool XtensaAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                                              OperandVector &Operands,
                                              MCStreamer &Out,
                                              uint64_t &ErrorInfo,
                                              bool MatchingInlineAsm) {
  MCInst Inst;

  unsigned Res =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm);

  switch (Res) {
  case Match_Success:
    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;

  case Match_MissingFeature:
    return Error(IDLoc, "instruction use requires an option to be enabled");

  case Match_MnemonicFail: {
    FeatureBitset FBS = ComputeAvailableFeatures(getSTI().getFeatureBits());
    std::string Suggestion = XtensaMnemonicSpellCheck(
        ((XtensaOperand &)*Operands[0]).getToken(), FBS);
    return Error(IDLoc, "invalid instruction" + Suggestion);
  }

  case Match_InvalidOperand: {
    if (ErrorInfo != ~0ULL) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");
    }

    return Error(refineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "invalid operand for instruction");
  }
  }

  if (const char *Diagnostic = getMatchKindDiag((XtensaMatchResultTy)Res)) {
    return Error(refineErrorLoc(IDLoc, Operands, ErrorInfo), Diagnostic);
  }

  llvm_unreachable("Implement any new match types added!");
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaAsmParser() {
  RegisterMCAsmParser<XtensaAsmParser> X(getTheXtensaTarget());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "XtensaGenAsmMatcher.inc"
