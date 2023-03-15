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
def movin_imm7 : ImmOperand<i32, "MoviNImm7">;


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

// Basic instruction formats
class XtensaInst<dag outs, dag ins, string asmstr, list<dag> pattern>
   : Instruction {
  let Namespace = "Xtensa";

  let OutOperandList = outs;
  let InOperandList  = ins;

  let AsmString   = asmstr;
  let Pattern     = pattern;
}

class XtensaGenericInstruction : GenericInstruction {
  let Namespace = "Xtensa";
}

class XtensaPseudo<dag outs, dag ins, list<dag> pattern>
   : XtensaInst<outs, ins, "", pattern> {
  let isCodeGenOnly = 1;
  let isPseudo = 1;
}

class Inst16<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : XtensaInst<outs, ins, asmstr, pattern> {
  field bits<16> Inst;

  let Size = 2;
  let Inst{3...0} = op0;
}

class Inst24<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : XtensaInst<outs, ins, asmstr, pattern> {
  field bits<24> Inst;

  let Size = 3;
  let Inst{3...0} = op0;
}

class I_Fixed16<bits<16> inst, string asmstr, list<dag> pattern>
   : XtensaInst<(outs), (ins), asmstr, pattern> {
  field bits<16> Inst = inst;
  let Size = 2;
}

class I_Fixed24<bits<24> inst, string asmstr, list<dag> pattern>
   : XtensaInst<(outs), (ins), asmstr, pattern> {
  field bits<24> Inst = inst;
  let Size = 3;
}

class I_RRR<bits<4> op0, bits<4> op1, bits<4> op2, dag outs, dag ins,
            string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> at;
  bits<4> as;
  bits<4> ar;

  let Inst{19...16} = op1;
  let Inst{23...20} = op2;

  let Inst{7...4} = at;
  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_RRI4<bits<4> op0, bits<4> op1, dag outs, dag ins, string asmstr,
             list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> imm4;

  bits<4> at;
  bits<4> as;
  bits<4> ar;

  let Inst{19...16} = op1;
  let Inst{23...20} = imm4;

  let Inst{7...4} = at;
  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_RRI8<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<8> imm8;

  bits<4> at;
  bits<4> as;
  bits<4> ar;

  let Inst{23...16} = imm8;

  let Inst{7...4} = at;
  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_RI16<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<16> imm16;
  bits<4> at;

  let Inst{23...8} = imm16;
  let Inst{7...4} = at;
}

class I_RSR<bits<4> op0, bits<4> op1, bits<4> op2, dag outs, dag ins,
            string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> at;
  bits<8> rs;

  let Inst{19...16} = op1;
  let Inst{23...20} = op2;

  let Inst{7...4} = at;
  let Inst{15...8} = rs;
}

class I_CALL<bits<4> op0, bits<2> n, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<18> offset;

  let Inst{5...4} = n;
  let Inst{23...6} = offset;
}

class I_CALLX<bits<4> op0, bits<4> op1, bits<4> op2, bits<2> m, bits<2> n,
              dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> ar;
  bits<4> as;

  let Inst{19...16} = op1;
  let Inst{23...20} = op2;

  let Inst{5...4} = n;
  let Inst{7...6} = m;

  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_BRI8<bits<4> op0, bits<2> m, bits<2> n, dag outs, dag ins,
             string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<8> offset;

  bits<4> ar;
  bits<4> as;

  let Inst{23...16} = offset;

  let Inst{5...4} = n;
  let Inst{7...6} = m;

  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_BRI12<bits<4> op0, bits<2> m, bits<2> n, dag outs, dag ins,
             string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<12> offset;
  bits<4> as;

  let Inst{23...12} = offset;

  let Inst{5...4} = n;
  let Inst{7...6} = m;

  let Inst{11...8} = as;
}

class I_RRRN<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<4> at;
  bits<4> as;
  bits<4> ar;

  let Inst{7...4} = at;
  let Inst{11...8} = as;
  let Inst{15...12} = ar;
}

class I_RI7<bits<4> op0, bit i, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<7> imm7;
  bits<4> as;

  let Inst{7} = i;
  let Inst{6...4} = imm7{6...4};
  let Inst{15...12} = imm7{3...0};
  let Inst{11...8} = as;
}

class I_RI6<bits<4> op0, bit i, bit z, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<6> imm6;
  bits<4> as;

  let Inst{7} = i;
  let Inst{6} = z;
  let Inst{5...4} = imm6{5...4};
  let Inst{15...12} = imm6{3...0};
  let Inst{11...8} = as;
}

// Arithmetic instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = false in {
  class ArithBaseRRR<bits<4> op1, bits<4> op2, dag outs, dag ins, string asmstr,
                     list<dag> pattern>
    : I_RRR<0b0000, op1, op2, outs, ins, asmstr, pattern>;

  class ArithBaseRRI8<bits<4> op0, bits<4> r, dag outs, dag ins, string asmstr,
                      list<dag> pattern>
    : I_RRI8<op0, outs, ins, asmstr, pattern> {
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

  class ArithBaseRRRN<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
    : I_RRRN<op0, outs, ins, asmstr, pattern>;

  class ArithRI7N<bits<4> op0, bit i, Operand Imm, string name, list<dag> pattern>
    : I_RI7<op0, i, (outs GPR:$as), (ins Imm:$imm7),
            !strconcat(name, "\t$as, $imm7"), pattern>;
}

class ArithRRR<bits<4> op1, bits<4> op2, string name, list<dag> pattern>
  : ArithBaseRRR<op1, op2, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
          !strconcat(name, "\t$ar, $as, $at"), pattern>;

class ArithRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
  : ArithBaseRRI8<op0, r, (outs GPR:$at), (ins GPR:$as, Imm:$imm8),
                  !strconcat(name, "\t$at, $as, $imm8"), pattern> {
  let isReMaterializable = true;
}

class MovRRR<bits<4> op2, string name, list<dag> pattern>
  : ArithBaseRRR<0b0011, op2, (outs GPR:$ar), (ins GPR:$_ar, GPR:$as, GPR:$at),
          !strconcat(name, "\t$ar, $as, $at"), pattern> {
  let Constraints = "$ar = $_ar";
}

class ArithRRrt<bits<4> op1, bits<4> op2, bits<4> s, string name, list<dag> pattern>
  : ArithBaseRRR<op1, op2, (outs GPR:$ar), (ins GPR:$at),
          !strconcat(name, "\t$ar, $at"), pattern> {
  let as = s;
}

class ArithRRrs<bits<4> op1, bits<4> op2, bits<4> t, string name, list<dag> pattern>
  : ArithBaseRRR<op1, op2, (outs GPR:$ar), (ins GPR:$as),
          !strconcat(name, "\t$ar, $as"), pattern> {
  let at = t;
}

class ArithRs<bits<4> op1, bits<4> op2, bits<4> t, bits<4> r, string name, list<dag> pattern>
  : ArithBaseRRR<op1, op2, (outs), (ins GPR:$as),
          !strconcat(name, "\t$as"), pattern> {
  let at = t;
  let ar = r;
}

class ArithRRRN<bits<4> op0, string name, list<dag> pattern>
  : ArithBaseRRRN<op0, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
          !strconcat(name, "\t$ar, $as, $at"), pattern>;

// Load instruction classes
let hasSideEffects = false, mayLoad = true, mayStore = false in {
  class LoadRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : ArithRRI8<op0, r, Imm, name, pattern> {
      let isReMaterializable = false;
    }

  class LoadRRRN<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : ArithBaseRRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$at, $as, $offset"), pattern> {
    bits<4> offset;
    let ar = offset;
  }

  class LoadRI16<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : I_RI16<op0, (outs GPR:$at), (ins Imm:$imm16),
             !strconcat(name, "\t$at, $imm16"), pattern>;
}

// Store instruction classes
let mayLoad = false, mayStore = true in {
  class StoreRRI8<bits<4> op0, bits<4> r, Operand Imm, string name, list<dag> pattern>
    : ArithBaseRRI8<op0, r, (outs), (ins GPR:$at, GPR:$as, Imm:$offset),
                    !strconcat(name, "\t$at, $as, $offset"), pattern>;

  class StoreRRRN<bits<4> op0, Operand Imm, string name, list<dag> pattern>
    : ArithBaseRRRN<op0, (outs), (ins GPR:$at, GPR:$as, Imm:$offset),
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

  class GenericCall<bits<4> op0, bits<2> n, Operand Imm, string name, list<dag> pattern>
    : I_CALL<op0, n, (outs), (ins Imm:$offset),
              !strconcat(name, "\t$offset"), pattern>;

  class GenericCallX<bits<2> m, bits<2> n, string name, list<dag> pattern>
      : I_CALLX<0b0000, 0b0000, 0b0000, m, n, (outs), (ins GPR:$as),
                !strconcat(name, "\t$as"), pattern> {
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
     : GenericCall<0b0101, n, calltarget18, name, []> {
    let Defs = [retAddr];
  }

  class CallX<bits<2> n, Register retAddr, string name>
     : GenericCallX<0b11, n, name, []> {
    let Defs = [retAddr];
  }
}
