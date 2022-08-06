include "XtensaInstrFormats.td"

// 8-bit immediate sign-extended to 32 bits
def simm8 : Operand<i32> {
  // TODO: define these
  // let ParserMatchClass = SImm8Operand;
  // let DecoderMethod = "DecodeSImm<8>";
}

// 8-bit immediate, shifted left by 8 and sign-extended to 32 bits
def simm8sl8 : Operand<i32> {
  // TODO: define these
  // let ParserMatchClass = SImm8Sl8Operand;
  // let DecoderMethod = "DecodeSImm<8>";
}

// 8-bit immediate, sign-extended to 32 bits and added to PC+4
def brtarget8 : Operand<OtherVT> {
  let OperandType = "OPERAND_PCREL";
  // TODO: define these
  // let EncoderMethod = "getBranchTargetOpValue";
  // let DecoderMethod = "DecodeBranchTarget";
  // let ParserMatchClass = MipsJumpTargetAsmOperand;
  // let PrintMethod = "printBranchOperand";
}

def b4const : Operand<i32> {
  // TODO: define these
  // let ParserMatchClass = B4ConstOperand;
  // let DecoderMethod = "DecodeB4Const";
}

def b4constu : Operand<i32> {
  // TODO: define these
  // let ParserMatchClass = B4ConstUOperand;
  // let DecoderMethod = "DecodeB4ConstU";
}

class ArithRRR<string name, bits<4> op1, bits<4> op2>
   : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
           !strconcat(name, "\t$ar, $as, $at"), []>;

class ArithRR<string name, bits<4> op1, bits<4> op2, bits<4> s>
   : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$at),
           !strconcat(name, "\t$ar, $at"), []> {
  let as = s;
}

class ArithRRI8<string name, bits<4> op0, bits<4> r, Operand Imm>
   : I_RRI8<op0, (outs GPR:$at), (ins GPR:$as, Imm:$imm8),
            !strconcat(name, "\t$at, $as, $imm8"), []> {
  let ar = r;
}

class BranchRR8<string name, bits<4> r>
   : I_RRI8<0b0111, (outs), (ins GPR:$as, GPR:$at, brtarget8:$offset),
            !strconcat(name, "\t$at, $as, $offset"), []>  {
  bits<8> offset;

  let imm8 = offset;
  let ar = r;
}

class BranchRR8C<string name, bits<2> m, bits<2> n, Operand Imm>
   : I_BRI8<0b0111, m, n, (outs), (ins GPR:$as, Imm:$imm, brtarget8:$offset),
            !strconcat(name, "\t$as, $imm, $offset"), []>;

class BranchRR8I<string name, bits<2> m> : BranchRR8C<name, m, 0b10, b4const>;
class BranchRR8UI<string name, bits<2> m> : BranchRR8C<name, m, 0b11, b4constu>;

def ABS : ArithRR<"abs", 0b0110, 0b0110, 0b0001>;

def ADD : ArithRRR<"add", 0b0000, 0b1000>;
def ADDI : ArithRRI8<"addi", 0b0010, 0b1100, simm8>;
def ADDMI : ArithRRI8<"addmi", 0b0010, 0b1101, simm8sl8>;
def ADDX2 : ArithRRR<"addx2", 0b0000, 0b1001>;
def ADDX4 : ArithRRR<"addx4", 0b0000, 0b1010>;
def ADDX8 : ArithRRR<"addx8", 0b0000, 0b1011>;

def AND : ArithRRR<"and", 0b0000, 0b0001>;

def BALL : BranchRR8<"ball", 0b0100>;
def BANY : BranchRR8<"bany", 0b1000>;
def BBC : BranchRR8<"bbc", 0b0101>;
// def BBCI : big bad cancer infestation
def BBS : BranchRR8<"bbs", 0b1101>;
// def BBSI : big bad sad infestation
def BEQ : BranchRR8<"beq", 0b0001>;
def BEQI : BranchRR8I<"beqi", 0b00>;
// def BEQZ
def BGE : BranchRR8<"bge", 0b1010>;
def BGEI : BranchRR8I<"begi", 0b11>;
def BGEU : BranchRR8<"bgeu", 0b1011>;
def BGEUI : BranchRR8UI<"bgeui", 0b11>;
// def BGEZ
def BLT : BranchRR8<"blt", 0b0010>;
def BLTI : BranchRR8I<"blti", 0b10>;
def BLTU : BranchRR8<"bltu", 0b0011>;
def BLTUI : BranchRR8UI<"bltui", 0b10>;
// def BLTZ
def BNALL : BranchRR8<"bnall", 0b1100>;
def BNE : BranchRR8<"bne", 0b1001>;
def BNEI : BranchRR8I<"bnei", 0b01>;
// def BNEZ
def BNONE : BranchRR8<"bnone", 0b0000>;

