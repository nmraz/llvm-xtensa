include "XtensaInstrFormats.td"

let OperandType = "OPERAND_IMMEDIATE" in {
  // 4-bit immediate, shifted left by 2 and zero-extended to 32 bits
  def uimm4sl2 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 8-bit immediate sign-extended to 32 bits
  def simm8 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 8-bit immediate zero-extended to 32 bits
  def uimm8 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 8-bit immediate, shifted left by 1 and zero-extended to 32 bits
  def uimm8sl1 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Sl8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 8-bit immediate, shifted left by 2 and zero-extended to 32 bits
  def uimm8sl2 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Sl8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 8-bit immediate, shifted left by 8 and sign-extended to 32 bits
  def simm8sl8 : Operand<i32> {
    // TODO: define these
    // let ParserMatchClass = SImm8Sl8Operand;
    // let DecoderMethod = "DecodeSImm<8>";
  }

  // 12-bit immediate zero-extended to 32 bits, used only in `entry` instruction
  def uimm12 : Operand<i32> {
    // TODO: parser/encoder/decoder
  }
}


let OperandType = "OPERAND_PCREL" in {
  // 8-bit immediate, sign-extended to 32 bits and added to PC+4
  def brtarget8 : Operand<OtherVT> {
    // TODO: define these
    // let EncoderMethod = "getBranchTargetOpValue";
    // let DecoderMethod = "DecodeBranchTarget";
    // let ParserMatchClass = MipsJumpTargetAsmOperand;
    // let PrintMethod = "printBranchOperand";
  }

  // 12-bit immediate, sign-extended to 32 bits and added to PC+4
  def brtarget12 : Operand<OtherVT> {
    // TODO: define these
    // let EncoderMethod = "getBranchTargetOpValue";
    // let DecoderMethod = "DecodeBranchTarget";
    // let ParserMatchClass = MipsJumpTargetAsmOperand;
    // let PrintMethod = "printBranchOperand";
  }

  // 18-bit immediate, shifted left by 2, sign extended to 32 bits and added to
  // PC+4
  def calltarget18 : Operand<OtherVT> {
    // TODO: define these
    // let EncoderMethod = "getBranchTargetOpValue";
    // let DecoderMethod = "DecodeBranchTarget";
    // let ParserMatchClass = MipsJumpTargetAsmOperand;
    // let PrintMethod = "printBranchOperand";
  }
}

let OperandType = "OPERAND_IMMEDIATE" in {
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
}


class ArithRRR<string name, bits<4> op1, bits<4> op2>
   : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
           !strconcat(name, "\t$ar, $as, $at"), []>;

class ArithRRs<string name, bits<4> op1, bits<4> op2, bits<4> s>
   : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$at),
           !strconcat(name, "\t$ar, $at"), []> {
  let as = s;
}

class ArithRRt<string name, bits<4> op1, bits<4> op2, bits<4> t>
   : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as),
           !strconcat(name, "\t$ar, $as"), []> {
  let at = t;
}

class ArithRRI8<string name, bits<4> op0, bits<4> r, Operand Imm>
   : I_RRI8<op0, (outs GPR:$at), (ins GPR:$as, Imm:$imm8),
            !strconcat(name, "\t$at, $as, $imm8"), []> {
  let ar = r;
}

class LoadRRI8<string name, bits<4> op0, bits<4> r, Operand Imm>
   : ArithRRI8<name, op0, r, Imm> {
  let mayLoad = true;
}

class StoreRRI8<string name, bits<4> op0, bits<4> r, Operand Imm>
   : ArithRRI8<name, op0, r, Imm> {
  let mayStore = true;
}

class ArithRRRN<string name, bits<4> op0>
   : I_RRRN<op0, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
            !strconcat(name, "\t$ar, $as, $at"), []>;

class LoadRRRN<string name, bits<4> op0, Operand Imm>
   : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
            !strconcat(name, "\t$at, $as, $offset"), []> {
  bits<4> offset;
  let ar = offset;
  let mayLoad = true;
}

class StoreRRRN<string name, bits<4> op0, Operand Imm>
   : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
            !strconcat(name, "\t$at, $as, $offset"), []> {
  bits<4> offset;
  let ar = offset;
  let mayStore = true;
}

class GenericBRI12<string name, bits<4> op0, bits<2> m, bits<2> n, Operand Imm>
  : I_BRI12<op0, m, n, (outs), (ins GPR:$as, Imm:$offset),
            !strconcat(name, "\t$as, $offset"), []>;

let isBranch = true, isTerminator = true in {
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

  class BranchRR12Z<string name, bits<2> m>
    : GenericBRI12<name, 0b0110, m, 0b01, brtarget12>;
}

class BranchRR8I<string name, bits<2> m> : BranchRR8C<name, m, 0b10, b4const>;
class BranchRR8UI<string name, bits<2> m> : BranchRR8C<name, m, 0b11, b4constu>;

class GenericCall<string name, bits<4> op0, bits<2> n>
   : I_CALL<op0, n, (outs), (ins calltarget18:$offset),
            !strconcat(name, "\t$offset"), []>;

class GenericCallX<string name, bits<2> m, bits<2> n>
     : I_CALLX<0b0000, 0b0000, 0b0000, m, n, (outs), (ins GPR:$as),
              !strconcat(name, "\t$as"), []> {
  let ar = 0b0000;
}

let isCall = true in {
  class Call<string name, bits<2> n>
     : GenericCall<name, 0b0101, n>;

  class CallX<string name, bits<2> n>
     : GenericCallX<name, 0b11, n>;
}


def ABS : ArithRRs<"abs", 0b0110, 0b0110, 0b0001>;

def ADD   : ArithRRR<"add", 0b0000, 0b1000>;
def ADDI  : ArithRRI8<"addi", 0b0010, 0b1100, simm8>;
def ADDMI : ArithRRI8<"addmi", 0b0010, 0b1101, simm8sl8>;
def ADDX2 : ArithRRR<"addx2", 0b0000, 0b1001>;
def ADDX4 : ArithRRR<"addx4", 0b0000, 0b1010>;
def ADDX8 : ArithRRR<"addx8", 0b0000, 0b1011>;

def ADDN : ArithRRRN<"add.n", 0b1010>;

def AND : ArithRRR<"and", 0b0000, 0b0001>;

def BALL : BranchRR8<"ball", 0b0100>;
def BANY : BranchRR8<"bany", 0b1000>;
def BBC : BranchRR8<"bbc", 0b0101>;
// def BBCI
def BBS : BranchRR8<"bbs", 0b1101>;
// def BBSI
def BEQ : BranchRR8<"beq", 0b0001>;
def BEQI : BranchRR8I<"beqi", 0b00>;
def BEQZ : BranchRR12Z<"beqz", 0b00>;
def BGE : BranchRR8<"bge", 0b1010>;
def BGEI : BranchRR8I<"begi", 0b11>;
def BGEU : BranchRR8<"bgeu", 0b1011>;
def BGEUI : BranchRR8UI<"bgeui", 0b11>;
def BGEZ : BranchRR12Z<"bgez", 0b11>;
def BLT : BranchRR8<"blt", 0b0010>;
def BLTI : BranchRR8I<"blti", 0b10>;
def BLTU : BranchRR8<"bltu", 0b0011>;
def BLTUI : BranchRR8UI<"bltui", 0b10>;
def BLTZ : BranchRR12Z<"bltz", 0b10>;
def BNALL : BranchRR8<"bnall", 0b1100>;
def BNE : BranchRR8<"bne", 0b1001>;
def BNEI : BranchRR8I<"bnei", 0b01>;
def BNEZ : BranchRR12Z<"bnez", 0b01>;
def BNONE : BranchRR8<"bnone", 0b0000>;

def CALL0  : Call<"call0", 0b00>;
def CALL4  : Call<"call4", 0b01>;
def CALL8  : Call<"call8", 0b10>;
def CALL12 : Call<"call12", 0b11>;

def CALLX0  : CallX<"callx0", 0b00>;
def CALLX4  : CallX<"callx4", 0b01>;
def CALLX8  : CallX<"callx8", 0b10>;
def CALLX12 : CallX<"callx12", 0b11>;

def DSYNC : I_Fixed24<0x002030, "dsync", []>;

def ENTRY : GenericBRI12<"entry", 0b0110, 0b00, 0b11, uimm12>;

def ESYNC : I_Fixed24<0x002020, "esync", []>;

// def EXTUI

def EXTW : I_Fixed24<0x0020d0, "extw", []>;

let isTrap = 1 in {
  def ILL : I_Fixed24<0x000000, "ill", []>;
  def ILLN : I_Fixed16<0xf06d, "ill.n", []>;
}

def ISYNC : I_Fixed24<0x002000, "isync", []>;

let isBranch = true, isTerminator = true, isBarrier = true in {
  def J : GenericCall<"j", 0b0110, 0b00>;
  // def J.L
  def JX : GenericCallX<"jx", 0b10, 0b10>;
}

def L8UI : LoadRRI8<"l8ui", 0b0010, 0b0000, uimm8>;
def L16SI : LoadRRI8<"l16si", 0b0010, 0b1001, uimm8sl1>;
def L16UI : LoadRRI8<"l16ui", 0b0010, 0b0001, uimm8sl1>;
def L32I : LoadRRI8<"l32i", 0b0010, 0b0010, uimm8sl2>;
def L32IN : LoadRRRN<"l32i.n", 0b1000, uimm4sl2>;
// def L32R

def MEMW : I_Fixed24<0x0020c0, "memw", []>;

// def MOV : InstAlias<"mov $ar, $as", (OR GPR:$ar, GPR:$as, GPR:$as)>;

let isSelect = 1 in {
  def MOVEQZ : ArithRRR<"moveqz", 0b0011, 0b1000>;
  def MOVNEZ : ArithRRR<"movgez", 0b0011, 0b1001>;
  def MOVLTZ : ArithRRR<"movgez", 0b0011, 0b1010>;
  def MOVGEZ : ArithRRR<"movgez", 0b0011, 0b1011>;
}

// def MOVI
// def MOVIN

def MUL16S : ArithRRR<"mul16s", 0b0001, 0b1101>;
def MUL16U : ArithRRR<"mul16u", 0b0001, 0b1100>;

def MULL : ArithRRR<"mull", 0b0010, 0b1000>;
def MULSH : ArithRRR<"mulsh", 0b0010, 0b1011>;
def MULUH : ArithRRR<"muluh", 0b0010, 0b1010>;

def NEG : ArithRRs<"neg", 0b0000, 0b0110, 0b0000>;

def NOP : I_Fixed24<0x0020f0, "nop", []>;
def NOPN : I_Fixed16<0xf03d, "nop.n", []>;

def OR : ArithRRR<"or", 0b0000, 0b0010>;

def QUOS : ArithRRR<"quos", 0b0010, 0b1101>;
def QUOU : ArithRRR<"quou", 0b0010, 0b1100>;
def REMS : ArithRRR<"rems", 0b0010, 0b1111>;
def REMU : ArithRRR<"remu", 0b0010, 0b1110>;

// def RER

let isTerminator = true, isBarrier = true in {
  def RET : I_Fixed24<0x000080, "ret", []>;
  def RETN : I_Fixed16<0xf00d, "ret.n", []>;
  def RETW : I_Fixed24<0x000090, "retw", []>;
  def RETWN : I_Fixed16<0xf01d, "retw.n", []>;
}

// def RSR

def RSYNC : I_Fixed24<0x002010, "rsync", []>;

// def RUR

def S8I : StoreRRI8<"s8i", 0b0010, 0b0100, uimm8>;
def S16I : StoreRRI8<"s16i", 0b0010, 0b0101, uimm8sl1>;
def S32I : StoreRRI8<"s32i", 0b0010, 0b0101, uimm8sl2>;
def S32IN : StoreRRRN<"s32i.n", 0b1001, uimm4sl2>;

// def S32NB

def SALT : ArithRRR<"salt", 0b0010, 0b0111>;
def SALTU : ArithRRR<"saltu", 0b0010, 0b0110>;

def SLL : ArithRRt<"sll", 0b0001, 0b1010, 0b0000>;
// def SLLI
def SRA : ArithRRs<"sra", 0b0001, 0b1011, 0b0000>;
// def SRAI
def SRC : ArithRRR<"src", 0b0001, 0b1000>;
def SRL : ArithRRs<"srl", 0b0001, 0b1001, 0b0000>;
// def SRLI

// def SSA8B
// def SSA8L
// def SSAI
// def SSL
// def SSR

def SUB : ArithRRR<"sub", 0b0000, 0b1100>;
def SUBX2 : ArithRRR<"subx2", 0b0000, 0b1101>;
def SUBX4 : ArithRRR<"subx4", 0b0000, 0b1110>;
def SUBX8 : ArithRRR<"subx8", 0b0000, 0b1111>;

// def WER
// def WSR
// def WUR

def XOR : ArithRRR<"xor", 0b0000, 0b0011>;

// def XSR
