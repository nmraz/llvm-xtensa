include "XtensaInstrFormats.td"

class ConstantImmOperandClass<string name, string diag> : AsmOperandClass {
  let Name = name;
  let ParserMethod = "parseConstantImm";
  let RenderMethod = "addImmOperands";
  let DiagnosticString = diag;
}

def ExprImmOperandClass : AsmOperandClass {
  let Name = "ExprImm";
  let ParserMethod = "parseExprImm";
  let PredicateMethod = "isImm";
  let RenderMethod = "addImmOperands";
}

class ConstantImmWithNameOperandClass<string name, string friendlyName>
   : ConstantImmOperandClass<name, !strconcat("invalid ", friendlyName, " immediate")>;

class ShiftedUImmOperandClass<string name, string friendlyName, int width, int shift>
   : ConstantImmWithNameOperandClass<name, friendlyName> {
  let PredicateMethod = "isShiftedUImm<" # width # ", " # shift # ">";
}

class ShiftedSImmOperandClass<string name, string friendlyName, int width, int shift>
   : ConstantImmWithNameOperandClass<name, friendlyName> {
  let PredicateMethod = "isShiftedSImm<" # width # ", " # shift # ">";
}


class OperandWithClass<ValueType ty, string className> : Operand<ty> {
  let ParserMatchClass = !cast<AsmOperandClass>(className # "OperandClass");
}

class PCRelOperand<ValueType ty> : OperandWithClass<ty, "ExprImm"> {
  let OperandType = "OPERAND_PCREL";
}

class ImmOperand<ValueType ty, string className> : OperandWithClass<ty, className> {
  let OperandType = "OPERAND_IMMEDIATE";
}

class UImmPatLeaf<ValueType ty, int width, int shift> : ImmLeaf<ty, "return isShiftedUInt<" # width # ", " # shift # ">(Imm);">;
class SImmPatLeaf<ValueType ty, int width, int shift> : ImmLeaf<ty, "return isShiftedInt<" # width # ", " # shift # ">(Imm);">;

class UImmOperand<ValueType ty, int width> : ImmOperand<ty, "UImm" # width>, UImmPatLeaf<ty, width, 0>;
class SImmOperand<ValueType ty, int width> : ImmOperand<ty, "SImm" # width>, SImmPatLeaf<ty, width, 0>;

class ShiftedUImmOperand<ValueType ty, int width, int shift>
   : ImmOperand<ty, "UImm" # width # "Sl" # shift>, UImmPatLeaf<ty, width, shift> {
  let EncoderMethod = "getShiftedImmOpValue<" # shift # ">";
}

class ShiftedSImmOperand<ValueType ty, int width, int shift>
   : ImmOperand<ty, "SImm" # width # "Sl" # shift>, SImmPatLeaf<ty, width, shift> {
  let EncoderMethod = "getShiftedImmOpValue<" # shift # ">";
}


// "Simple" operands

// 4-bit immediate
def UImm4OperandClass : ShiftedUImmOperandClass<"UImm4", "4-bit", 4, 0>;
def uimm4 : UImmOperand<i32, 4>;

// 4-bit immediate, shifted left by 2 and zero-extended to 32 bits
def UImm4Sl2OperandClass : ShiftedUImmOperandClass<"UImm4Sl2", "shifted 4-bit", 4, 2>;
def uimm4_sl2 : ShiftedUImmOperand<i32, 4, 2>;

// 5-bit immediate zero-extended to 32 bits
def UImm5OperandClass : ShiftedUImmOperandClass<"UImm5", "5-bit", 5, 0>;
def uimm5 : UImmOperand<i32, 5>;

// 8-bit immediate sign-extended to 32 bits
def SImm8OperandClass : ShiftedSImmOperandClass<"SImm8", "signed 8-bit", 8, 0>;
def simm8 : SImmOperand<i32, 8>;

// 8-bit immediate zero-extended to 32 bits
def UImm8OperandClass : ShiftedUImmOperandClass<"UImm8", "8-bit", 8, 0>;
def uimm8 : UImmOperand<i32, 8>;

// 8-bit immediate, shifted left by 1 and zero-extended to 32 bits
def UImm8Sl1OperandClass : ShiftedUImmOperandClass<"UImm8Sl1", "shifted 8-bit", 8, 1>;
def uimm8_sl1 : ShiftedUImmOperand<i32, 8, 1>;

// 8-bit immediate, shifted left by 2 and zero-extended to 32 bits
def UImm8Sl2OperandClass : ShiftedUImmOperandClass<"UImm8Sl2", "shifted 8-bit", 8, 2>;
def uimm8_sl2 : ShiftedUImmOperand<i32, 8, 2>;

// 8-bit immediate, shifted left by 8 and sign-extended to 32 bits
def SImm8Sl8OperandClass : ShiftedSImmOperandClass<"SImm8Sl1", "shifted 8-bit", 8, 8>;
def simm8_sl8 : ShiftedSImmOperand<i32, 8, 8>;

// 12-bit immediate sign-extended to 32 bits
def SImm12OperandClass : ShiftedSImmOperandClass<"SImm12", "signed 12-bit", 12, 0>;
def simm12 : SImmOperand<i32, 12>;

// 12-bit immediate, shifted left by 3 zero-extended to 32 bits
def UImm12Sl3OperandClass : ShiftedUImmOperandClass<"UImm12Sl3", "shifted 12-bit", 12, 3>;
def uimm12_sl3 : ShiftedUImmOperand<i32, 12, 3>;


// "Special" operands

// 4-bit encoded immediate for signed branch comparisons
def B4ConstOperandClass : ConstantImmOperandClass<"B4Const", "invalid b4const operand">;
def b4const : ImmOperand<i32, "B4Const"> {
  let EncoderMethod = "getB4ConstOpValue";
}

// 4-bit encoded immediate for unsigned branch comparisons
def B4ConstUOperandClass : ConstantImmOperandClass<"B4ConstU", "invalid b4constu operand">;
def b4constu : ImmOperand<i32, "B4ConstU"> {
  let EncoderMethod = "getB4ConstUOpValue";
}

// 4-bit immediate incremented by 1 (used only in `extui`)
def UImm4Plus1OperandClass : ConstantImmOperandClass<"UImm4Plus1", "expected a constant in the range [1, 16]">;
def uimm4_plus1 : ImmOperand<i32, "UImm4Plus1"> {
  let EncoderMethod = "getUImm4Plus1OpValue";
}

// 5-bit immediate subtracted from 32; used only in `slli` instruction
def uimm5_sub32 : ImmOperand<i32, "UImm5">, ImmLeaf<i32, [{ return Imm >= 1 && Imm <= 31; }]> {
  let EncoderMethod = "getUImm5Sub32OpValue";
}

// 7-bit immediate, extended to 32 bits by the bitwise AND of its top 2 bits
// (bits 5 and 6); used only in narrow `movi` instruction
def MoviNImm7OperandClass : ConstantImmOperandClass<"MoviNImm7", "expected a constant in the range [-32, 95]">;
def movin_imm7 : ImmOperand<i32, "MoviNImm7">, ImmLeaf<i32, [{ return Imm >= -32 && Imm <= 95; }]>;


// PC-relative operands (branches and loads)

// 8-bit immediate, sign-extended to 32 bits and added to PC+4
def brtarget8 : PCRelOperand<OtherVT> {
  let EncoderMethod = "getBrTarget8OpValue";
  let PrintMethod = "printBranchOperand";
}

// 12-bit immediate, sign-extended to 32 bits and added to PC+4
def brtarget12 : PCRelOperand<OtherVT> {
  let EncoderMethod = "getBrTarget12OpValue";
  let PrintMethod = "printBranchOperand";
}

// 16-bit immediate, one-extended to signed 32 bits, shifted left by 2,
// and added to AlignUp(PC, 4)
def l32rtarget16 : PCRelOperand<i32> {
  let EncoderMethod = "getL32RTarget16OpValue";
  let PrintMethod = "printL32ROperand";
}

// 18-bit immediate, sign extended to 32 bits and added to PC+4
def jmptarget18 : PCRelOperand<OtherVT> {
  let EncoderMethod = "getJmpTarget18OpValue";
  let PrintMethod = "printBranchOperand";
}

// 18-bit immediate, shifted left by 2, sign extended to 32 bits and added to
// AlignDown(PC, 4) + 4
def calltarget18 : PCRelOperand<OtherVT> {
  let EncoderMethod = "getCallTarget18OpValue";
  let PrintMethod = "printCallOperand";
}


// Pattern matching helpers
def extui_maskimm_xform : SDNodeXForm<imm, [{ unimplemented }]>;
def gi_extui_maskimm_xform : GICustomOperandRenderer<"renderExtuiMaskImm">,
                             GISDNodeXFormEquiv<extui_maskimm_xform>;
def extui_maskimm : ImmLeaf<i32, [{
  return Imm <= 0xffff && isMask_64(Imm);
}], extui_maskimm_xform>;

def extui_lshrimm : ComplexPattern<i32, 2, "", []>;
def gi_extui_lshrimm : GIComplexOperandMatcher<s32, "selectExtuiLshrImm">,
                       GIComplexPatternEquiv<extui_lshrimm>;

def l32r_const_xform : SDNodeXForm<imm, [{ unimplemented }]>;
def gi_l32r_const_xform : GICustomOperandRenderer<"renderL32RConst">,
                          GISDNodeXFormEquiv<l32r_const_xform>;
def l32r_const : ImmLeaf<i32, [{ return !isInt<12>(Imm); }], l32r_const_xform>;


// Arithmetic instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = false in {
  class ArithRRR<bits<4> op1, bits<4> op2, string name, list<dag> pattern>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
            !strconcat(name, "\t$ar, $as, $at"), pattern>;

  class ArithRRrt<bits<4> op1, bits<4> op2, bits<4> s, string name, list<dag> pattern>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$at),
            !strconcat(name, "\t$ar, $at"), pattern> {
    let as = s;
  }

  class ArithRRrs<bits<4> op1, bits<4> op2, bits<4> t, string name, list<dag> pattern>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as),
            !strconcat(name, "\t$ar, $as"), pattern> {
    let at = t;
  }

  class ArithRs<bits<4> op1, bits<4> op2, bits<4> t, bits<4> r, string name, list<dag> pattern>
    : I_RRR<0b0000, op1, op2, (outs), (ins GPR:$as),
            !strconcat(name, "\t$as"), pattern> {
    let at = t;
    let ar = r;
  }

  class ArithRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : I_RRI8<op0, (outs GPR:$at), (ins GPR:$as, Imm:$imm8),
              !strconcat(name, "\t$at, $as, $imm8"), pattern> {
    let ar = r;
  }

  class ArithRI12<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : I_RRI8<op0, (outs GPR:$at), (ins Imm:$imm12),
             !strconcat(name, "\t$at, $imm12"), pattern> {
    bits<12> imm12;
    let ar = r;
    let as = imm12{11...8};
    let imm8 = imm12{7...0};
  }

  class ArithRRRN<bits<4> op0, string name, list<dag> pattern>
   : I_RRRN<op0, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
            !strconcat(name, "\t$ar, $as, $at"), pattern>;

  class ArithRI7N<bits<4> op0, bit i, Operand Imm, string name, list<dag> pattern>
    : I_RI7<op0, i, (outs GPR:$as), (ins Imm:$imm7),
            !strconcat(name, "\t$as, $imm7"), pattern>;
}

// Load instruction classes
let hasSideEffects = false, mayLoad = true, mayStore = false in {
  class LoadRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : ArithRRI8<op0, r, Imm, name, pattern>;

  class LoadRRRN<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$at, $as, $offset"), pattern> {
    bits<4> offset;
    let ar = offset;
  }

  class LoadRI16<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : I_RI16<op0, (outs GPR:$at), (ins Imm:$imm16),
             !strconcat(name, "\t$at, $imm16"), pattern>;
}

// Store instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = true in {
  class StoreRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : ArithRRI8<op0, r, Imm, name, pattern>;

  class StoreRRRN<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$at, $as, $offset"), pattern> {
    bits<4> offset;
    let ar = offset;
  }
}

// Branch and call helpers
let hasSideEffects = false, mayLoad = false, mayStore = false in {
  class GenericBRI12<bits<4> op0, bits<2> m, bits<2> n, Operand Imm, string name>
    : I_BRI12<op0, m, n, (outs), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$as, $offset"), []>;

  class GenericCall<bits<4> op0, bits<2> n, Operand Imm, string name>
    : I_CALL<op0, n, (outs), (ins Imm:$offset),
              !strconcat(name, "\t$offset"), []>;

  class GenericCallX<bits<2> m, bits<2> n, string name>
      : I_CALLX<0b0000, 0b0000, 0b0000, m, n, (outs), (ins GPR:$as),
                !strconcat(name, "\t$as"), []> {
    let ar = 0b0000;
  }
}

// Branch instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = false,
    isBranch = true, isTerminator = true in {
  class BranchRR8<bits<4> r, string name>
    : I_RRI8<0b0111, (outs), (ins GPR:$as, GPR:$at, brtarget8:$offset),
              !strconcat(name, "\t$at, $as, $offset"), []>  {
    bits<8> offset;

    let imm8 = offset;
    let ar = r;
  }

  class BranchRC8<bits<2> m, bits<2> n, Operand Imm, string name>
    : I_BRI8<0b0110, m, n, (outs), (ins GPR:$as, Imm:$imm, brtarget8:$offset),
              !strconcat(name, "\t$as, $imm, $offset"), []> {
    bits<4> imm;

    let ar = imm;
  }

  class BranchRB8<bits<3> arhi, string name>
    : I_BRI8<0b0111, 0b00, 0b00, (outs), (ins GPR:$as, uimm5:$bbi, brtarget8:$offset),
             !strconcat(name, "\t$as, $bbi, $offset"), []> {
    bits<5> bbi;

    let ar{0} = bbi{4};
    let ar{3...1} = arhi;

    let Inst{7...4} = bbi{3...0};
  }

  class BranchRZ12<bits<2> m, string name>
    : GenericBRI12<0b0110, m, 0b01, brtarget12, name>;
}

class BranchRI8<bits<2> m, string name> : BranchRC8<m, 0b10, b4const, name>;
class BranchRUI8<bits<2> m, string name> : BranchRC8<m, 0b11, b4constu, name>;

// Call instruction classes
let isCall = true in {
  class Call<bits<2> n, Register retAddr, string name>
     : GenericCall<0b0101, n, calltarget18, name> {
    let Defs = [retAddr];
  }

  class CallX<bits<2> n, Register retAddr, string name>
     : GenericCallX<0b11, n, name> {
    let Defs = [retAddr];
  }
}


let Uses = [A1], Defs = [A1] in {
  // Pseudo-instruction used to model allocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be reserved on the stack,
  // and `amt2` represents the number of bytes that have already been reserved
  // for the purposes of use in this call (beyond `amt1`).
  def ADJCALLSTACKDOWN : XtensaPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), []>;

  // Pseudo-instruction used to model deallocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be popped off the stack,
  // and `amt2` represents the number of bytes that have already been popped by
  // the caller.
  def ADJCALLSTACKUP : XtensaPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), []>;
}

def ABS : ArithRRrt<0b0000, 0b0110, 0b0001, "abs", []>;

def ADD   : ArithRRR<0b0000, 0b1000, "add", []>;
def ADDI  : ArithRRI8<0b0010, 0b1100, simm8, "addi", []>;
def ADDMI : ArithRRI8<0b0010, 0b1101, simm8_sl8, "addmi", []>;
def ADDX2 : ArithRRR<0b0000, 0b1001, "addx2", []>;
def ADDX4 : ArithRRR<0b0000, 0b1010, "addx4", []>;
def ADDX8 : ArithRRR<0b0000, 0b1011, "addx8", []>;

def ADDN : ArithRRRN<0b1010, "add.n", [(set i32:$ar, (add i32:$as, i32:$at))]>;

def AND : ArithRRR<0b0000, 0b0001, "and", [(set i32:$ar, (and i32:$as, i32:$at))]>;

def BALL : BranchRR8<0b0100, "ball">;
def BANY : BranchRR8<0b1000, "bany">;
def BBC : BranchRR8<0b0101, "bbc">;
def BBCI : BranchRB8<0b011, "bbci">;
def BBS : BranchRR8<0b1101, "bbs">;
def BBSI : BranchRB8<0b111, "bbsi">;
def BEQ : BranchRR8<0b0001, "beq">;
def BEQI : BranchRI8<0b00, "beqi">;
def BEQZ : BranchRZ12<0b00, "beqz">;
def BGE : BranchRR8<0b1010, "bge">;
def BGEI : BranchRI8<0b11, "bgei">;
def BGEU : BranchRR8<0b1011, "bgeu">;
def BGEUI : BranchRUI8<0b11, "bgeui">;
def BGEZ : BranchRZ12<0b11, "bgez">;
def BLT : BranchRR8<0b0010, "blt">;
def BLTI : BranchRI8<0b10, "blti">;
def BLTU : BranchRR8<0b0011, "bltu">;
def BLTUI : BranchRUI8<0b10, "bltui">;
def BLTZ : BranchRZ12<0b10, "bltz">;
def BNALL : BranchRR8<0b1100, "bnall">;
def BNE : BranchRR8<0b1001, "bne">;
def BNEI : BranchRI8<0b01, "bnei">;
def BNEZ : BranchRZ12<0b01, "bnez">;
def BNONE : BranchRR8<0b0000, "bnone">;

def CALL0  : Call<0b00, A0, "call0">;
def CALL4  : Call<0b01, A4, "call4">;
def CALL8  : Call<0b10, A8, "call8">;
def CALL12 : Call<0b11, A12, "call12">;

def CALLX0  : CallX<0b00, A0, "callx0">;
def CALLX4  : CallX<0b01, A4, "callx4">;
def CALLX8  : CallX<0b10, A8, "callx8">;
def CALLX12 : CallX<0b11, A12, "callx12">;

def DSYNC : I_Fixed24<0x002030, "dsync", []>;

def ENTRY : GenericBRI12<0b0110, 0b00, 0b11, uimm12_sl3, "entry"> {
  // This essentially renames all registers.
  let hasSideEffects = true;
}

def ESYNC : I_Fixed24<0x002020, "esync", []>;

// This instruction is so unusual that it's easiest to just define manually
def EXTUI
   : Inst24<0b0000, (outs GPR:$ar), (ins GPR:$at, uimm5:$shiftimm, uimm4_plus1:$maskimm),
            "extui $ar, $at, $shiftimm, $maskimm", []> {
  let hasSideEffects = false;
  let mayLoad = false;
  let mayStore = false;

  bits<4> ar;
  bits<4> at;

  bits<5> shiftimm;
  bits<4> maskimm;

  let Inst{7...4} = at;
  let Inst{11...8} = shiftimm{3...0};
  let Inst{15...12} = ar;
  let Inst{16} = shiftimm{4};
  let Inst{19...17} = 0b010;
  let Inst{23...20} = maskimm;
}

// Basic bit extraction patterns
def : Pat<(and i32:$at, extui_maskimm:$maskimm),
          (EXTUI i32:$at, 0, extui_maskimm:$maskimm)>;

// Right shift immediates that don't fit in 4 bits need an extui instead
def : Pat<(srl i32:$at, (extui_lshrimm i32:$lshrimm, i32:$maskimm)),
          (EXTUI i32:$at, i32:$lshrimm, i32:$maskimm)>;

def EXTW : I_Fixed24<0x0020d0, "extw", []>;

let isTrap = true in {
  def ILL : I_Fixed24<0x000000, "ill", []>;
  def ILLN : I_Fixed16<0xf06d, "ill.n", []>;
}

def ISYNC : I_Fixed24<0x002000, "isync", []>;

let isBranch = true, isTerminator = true, isBarrier = true in {
  def J : GenericCall<0b0110, 0b00, jmptarget18, "j">;
  // def J.L
  def JX : GenericCallX<0b10, 0b10, "jx">;
}

def L8UI : LoadRRI8<0b0010, 0b0000, uimm8, "l8ui", []>;
def L16SI : LoadRRI8<0b0010, 0b1001, uimm8_sl1, "l16si", []>;
def L16UI : LoadRRI8<0b0010, 0b0001, uimm8_sl1, "l16ui", []>;
def L32I : LoadRRI8<0b0010, 0b0010, uimm8_sl2, "l32i", []>;
def L32IN : LoadRRRN<0b1000, uimm4_sl2, "l32i.n", []>;
def L32R : LoadRI16<0b0001, l32rtarget16, "l32r", [(set i32:$at, l32r_const:$imm16)]>;

def MEMW : I_Fixed24<0x0020c0, "memw", []>;

let isSelect = 1 in {
  def MOVEQZ : ArithRRR<0b0011, 0b1000, "moveqz", []>;
  def MOVNEZ : ArithRRR<0b0011, 0b1001, "movnez", []>;
  def MOVLTZ : ArithRRR<0b0011, 0b1010, "movltz", []>;
  def MOVGEZ : ArithRRR<0b0011, 0b1011, "movgez", []>;
}

def MOVIN : ArithRI7N<0b1100, 0, movin_imm7, "movi.n", [(set i32:$as, movin_imm7:$imm7)]>;
def MOVI : ArithRI12<0b0010, 0b1010, simm12, "movi", [(set i32:$at, simm12:$imm12)]>;

def MUL16S : ArithRRR<0b0001, 0b1101, "mul16s", []>;
def MUL16U : ArithRRR<0b0001, 0b1100, "mul16u", []>;

def MULL : ArithRRR<0b0010, 0b1000, "mull", [(set i32:$ar, (mul i32:$as, i32:$at))]>;
def MULSH : ArithRRR<0b0010, 0b1011, "mulsh", [(set i32:$ar, (mulhs i32:$as, i32:$at))]>;
def MULUH : ArithRRR<0b0010, 0b1010, "muluh", [(set i32:$ar, (mulhu i32:$as, i32:$at))]>;

def NEG : ArithRRrt<0b0000, 0b0110, 0b0000, "neg", []>;

def NOP : I_Fixed24<0x0020f0, "nop", []>;
def NOPN : I_Fixed16<0xf03d, "nop.n", []>;

def OR : ArithRRR<0b0000, 0b0010, "or", [(set i32:$ar, (or i32:$as, i32:$at))]>;
def MOV : InstAlias<"mov $ar, $as", (OR GPR:$ar, GPR:$as, GPR:$as)>;

def QUOS : ArithRRR<0b0010, 0b1101, "quos", [(set i32:$ar, (sdiv i32:$as, i32:$at))]>;
def QUOU : ArithRRR<0b0010, 0b1100, "quou", [(set i32:$ar, (udiv i32:$as, i32:$at))]>;
def REMS : ArithRRR<0b0010, 0b1111, "rems", [(set i32:$ar, (srem i32:$as, i32:$at))]>;
def REMU : ArithRRR<0b0010, 0b1110, "remu", [(set i32:$ar, (urem i32:$as, i32:$at))]>;

// def RER

let Uses = [A0],
    isTerminator = true, isBarrier = true, isReturn = true,
    mayLoad = false, mayStore = false in {
  let hasSideEffects = false in {
    def RET : I_Fixed24<0x000080, "ret", []>;
    def RETN : I_Fixed16<0xf00d, "ret.n", []>;
  }

  // These instructions also have the side effect of incrementing the window
  // before performing the return.
  def RETW : I_Fixed24<0x000090, "retw", []>;
  def RETWN : I_Fixed16<0xf01d, "retw.n", []>;
}

// def RSR

def RSYNC : I_Fixed24<0x002010, "rsync", []>;

// def RUR

def S8I : StoreRRI8<0b0010, 0b0100, uimm8, "s8i", []>;
def S16I : StoreRRI8<0b0010, 0b0101, uimm8_sl1, "s16i", []>;
def S32I : StoreRRI8<0b0010, 0b0101, uimm8_sl2, "s32i", []>;
def S32IN : StoreRRRN<0b1001, uimm4_sl2, "s32i.n", []>;

// def S32NB

def SALT : ArithRRR<0b0010, 0b0111, "salt", [(set i32:$ar, (setlt i32:$as, i32:$at))]>;
def SALTU : ArithRRR<0b0010, 0b0110, "saltu", [(set i32:$ar, (setult i32:$as, i32:$at))]>;

let Uses = [SAR] in {
  def SLL : ArithRRrs<0b0001, 0b1010, 0b0000, "sll", []>;
  def SRA : ArithRRrt<0b0001, 0b1011, 0b0000, "sra", []>;
  def SRC : ArithRRR<0b0001, 0b1000, "src", []>;
  def SRL : ArithRRrt<0b0001, 0b1001, 0b0000, "srl", []>;
}

let hasSideEffects = false, mayLoad = false, mayStore = false in {
  def SLLI : I_RRR<0b0000, 0b0001, 0b0000, (outs GPR:$ar), (ins GPR:$as, uimm5_sub32:$imm),
                   "slli $ar, $as, $imm", [(set i32:$ar, (shl i32:$as, uimm5_sub32:$imm))]> {
    bits<5> imm;

    let at = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRAI : I_RRR<0b0000, 0b0001, 0b0010, (outs GPR:$ar), (ins GPR:$at, uimm5:$imm),
                   "srai $ar, $at, $imm", [(set i32:$ar, (sra i32:$at, uimm5:$imm))]> {
    bits<5> imm;

    let as = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRLI : I_RRR<0b0000, 0b0001, 0b0100, (outs GPR:$ar), (ins GPR:$at, uimm4:$imm),
                   "srli $ar, $at, $imm", [(set i32:$ar, (srl i32:$at, uimm4:$imm))]> {
    bits<4> imm;

    let as = imm;
  }
}

let Defs = [SAR] in {
  def SSA8B : ArithRs<0b0000, 0b0100, 0b0000, 0b0011, "ssa8b", []>;
  def SSA8L : ArithRs<0b0000, 0b0100, 0b0000, 0b0010, "ssa8l", []>;
  def SSL : ArithRs<0b0000, 0b0100, 0b0000, 0b0001, "ssl", []>;
  def SSR : ArithRs<0b0000, 0b0100, 0b0000, 0b0000, "ssr", []>;

  def SSAI : I_RRR<0b0000, 0b0000, 0b0100, (outs), (ins uimm5:$imm),
                  "ssai $imm", []> {
    let hasSideEffects = false;
    let mayLoad = false;
    let mayStore = false;

    bits<5> imm;

    let at = 0b0000;
    let at{0} = imm{4};
    let as = imm{3...0};
    let ar = 0b0100;
  }
}

def SUB : ArithRRR<0b0000, 0b1100, "sub", [(set i32:$ar, (sub i32:$as, i32:$at))]>;
def SUBX2 : ArithRRR<0b0000, 0b1101, "subx2", []>;
def SUBX4 : ArithRRR<0b0000, 0b1110, "subx4", []>;
def SUBX8 : ArithRRR<0b0000, 0b1111, "subx8", []>;

// def WER
// def WSR
// def WUR

def XOR : ArithRRR<0b0000, 0b0011, "xor", [(set i32:$ar, (xor i32:$as, i32:$at))]>;

// def XSR
