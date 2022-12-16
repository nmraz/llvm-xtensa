include "XtensaInstrFormats.td"

let OperandType = "OPERAND_IMMEDIATE" in {
  // unsigned 4-bit immediate
  def uimm4 : Operand<i4> {
  }

  // unsigned 4-bit immediate incremented by 1 (used only in `extui` instruction)
  def uimm4_plus1 : Operand<i4> {
  }

  // 4-bit immediate, shifted left by 2 and zero-extended to 32 bits
  def uimm4_sl2 : Operand<i32> {
  }

  // 5-bit immediate zero-extended to 32 bits
  def uimm5 : Operand<i32> {
  }

  // 5-bit immediate subtracted from 32
  def uimm5_sub32 : Operand<i32> {
  }

  // 7-bit immediate, extended to 32 bits by the bitwise AND of its top 2 bits
  // (bits 5 and 6); used only in narrow `movi` instruction
  def movin_imm7 : Operand<i32> {
  }

  // 8-bit immediate sign-extended to 32 bits
  def simm8 : Operand<i32> {
  }

  // 8-bit immediate zero-extended to 32 bits
  def uimm8 : Operand<i32> {
  }

  // 8-bit immediate, shifted left by 1 and zero-extended to 32 bits
  def uimm8_sl1 : Operand<i32> {
  }

  // 8-bit immediate, shifted left by 2 and zero-extended to 32 bits
  def uimm8_sl2 : Operand<i32> {
  }

  // 8-bit immediate, shifted left by 8 and sign-extended to 32 bits
  def simm8_sl8 : Operand<i32> {
  }

  // 12-bit immediate sign-extended to 32 bits
  def movi_simm12 : Operand<i32> {
  }

  // 12-bit immediate, shifted left by 3 zero-extended to 32 bits
  def uimm12_sl3 : Operand<i32> {
  }

  // Special 4-bit encoded immediate for signed branch comparisons
  def b4const : Operand<i32> {
  }

  // Special 4-bit encoded immediate for unsigned branch comparisons
  def b4constu : Operand<i32> {
  }
}

let OperandType = "OPERAND_PCREL" in {
  // 8-bit immediate, sign-extended to 32 bits and added to PC+4
  def brtarget8 : Operand<OtherVT> {
  }

  // 12-bit immediate, sign-extended to 32 bits and added to PC+4
  def brtarget12 : Operand<OtherVT> {
  }

  // 18-bit immediate, shifted left by 2, sign extended to 32 bits and added to
  // AlignDown(PC, 4) + 4
  def calltarget18 : Operand<OtherVT> {
  }
}


// Arithmetic instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = false in {
  class ArithRRR<bits<4> op1, bits<4> op2, string name>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
            !strconcat(name, "\t$ar, $as, $at"), []>;

  class ArithRRrt<bits<4> op1, bits<4> op2, bits<4> s, string name>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$at),
            !strconcat(name, "\t$ar, $at"), []> {
    let as = s;
  }

  class ArithRRrs<bits<4> op1, bits<4> op2, bits<4> t, string name>
    : I_RRR<0b0000, op1, op2, (outs GPR:$ar), (ins GPR:$as),
            !strconcat(name, "\t$ar, $as"), []> {
    let at = t;
  }

  class ArithRs<bits<4> op1, bits<4> op2, bits<4> t, bits<4> r, string name>
    : I_RRR<0b0000, op1, op2, (outs), (ins GPR:$as),
            !strconcat(name, "\t$as"), []> {
    let at = t;
    let ar = r;
  }

  class ArithRRI8<bits<4> op0, bits<4> r, Operand Imm, string name>
    : I_RRI8<op0, (outs GPR:$at), (ins GPR:$as, Imm:$imm8),
              !strconcat(name, "\t$at, $as, $imm8"), []> {
    let ar = r;
  }

  class ArithRI12<bits<4> op0, bits<4> r, Operand Imm, string name>
    : I_RRI8<op0, (outs GPR:$at), (ins Imm:$imm12),
             !strconcat(name, "\t$at, $imm12"), []> {
    bits<12> imm12;
    let ar = r;
    let as = imm12{11...8};
    let imm8 = imm12{7...0};
  }

  class ArithRRRN<bits<4> op0, string name>
   : I_RRRN<op0, (outs GPR:$ar), (ins GPR:$as, GPR:$at),
            !strconcat(name, "\t$ar, $as, $at"), []>;

  class ArithRI7N<bits<4> op0, bit i, Operand Imm, string name>
    : I_RI7<op0, i, (outs GPR:$as), (ins Imm:$imm7),
            !strconcat(name, "\t$as, $imm7"), []>;
}

// Load instruction classes
let hasSideEffects = false, mayLoad = true, mayStore = false in {
  class LoadRRI8<bits<4> op0, bits<4> r, Operand Imm, string name>
    : ArithRRI8<op0, r, Imm, name>;

  class LoadRRRN<bits<4> op0, Operand Imm, string name>
    : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$at, $as, $offset"), []> {
    bits<4> offset;
    let ar = offset;
  }
}

// Store instruction classes
let hasSideEffects = false, mayLoad = false, mayStore = true in {
  class StoreRRI8<bits<4> op0, bits<4> r, Operand Imm, string name>
    : ArithRRI8<op0, r, Imm, name>;

  class StoreRRRN<bits<4> op0, Operand Imm, string name>
    : I_RRRN<op0, (outs GPR:$at), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$at, $as, $offset"), []> {
    bits<4> offset;
    let ar = offset;
  }
}

// Branch and call helpers
let hasSideEffects = false, mayLoad = false, mayStore = false in {
  class GenericBRI12<bits<4> op0, bits<2> m, bits<2> n, Operand Imm, string name>
    : I_BRI12<op0, m, n, (outs), (ins GPR:$as, Imm:$offset),
              !strconcat(name, "\t$as, $offset"), []>;

  class GenericCall<bits<4> op0, bits<2> n, string name>
    : I_CALL<op0, n, (outs), (ins calltarget18:$offset),
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
    : I_BRI8<0b0111, m, n, (outs), (ins GPR:$as, Imm:$imm, brtarget8:$offset),
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
  class Call<bits<2> n, string name>
     : GenericCall<0b0101, n, name>;

  class CallX<bits<2> n, string name>
     : GenericCallX<0b11, n, name>;
}


def ABS : ArithRRrt<0b0000, 0b0110, 0b0001, "abs">;

def ADD   : ArithRRR<0b0000, 0b1000, "add">;
def ADDI  : ArithRRI8<0b0010, 0b1100, simm8, "addi">;
def ADDMI : ArithRRI8<0b0010, 0b1101, simm8_sl8, "addmi">;
def ADDX2 : ArithRRR<0b0000, 0b1001, "addx2">;
def ADDX4 : ArithRRR<0b0000, 0b1010, "addx4">;
def ADDX8 : ArithRRR<0b0000, 0b1011, "addx8">;

def ADDN : ArithRRRN<0b1010, "add.n">;

def AND : ArithRRR<0b0000, 0b0001, "and">;

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
def BGEI : BranchRI8<0b11, "begi">;
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

def CALL0  : Call<0b00, "call0">;
def CALL4  : Call<0b01, "call4">;
def CALL8  : Call<0b10, "call8">;
def CALL12 : Call<0b11, "call12">;

def CALLX0  : CallX<0b00, "callx0">;
def CALLX4  : CallX<0b01, "callx4">;
def CALLX8  : CallX<0b10, "callx8">;
def CALLX12 : CallX<0b11, "callx12">;

def DSYNC : I_Fixed24<0x002030, "dsync", []>;

def ENTRY : GenericBRI12<0b0110, 0b00, 0b11, uimm12_sl3, "entry"> {
  // We currently don't model `WindowBase` explicitly
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

def EXTW : I_Fixed24<0x0020d0, "extw", []>;

let isTrap = true in {
  def ILL : I_Fixed24<0x000000, "ill", []>;
  def ILLN : I_Fixed16<0xf06d, "ill.n", []>;
}

def ISYNC : I_Fixed24<0x002000, "isync", []>;

let isBranch = true, isTerminator = true, isBarrier = true in {
  def J : GenericCall<0b0110, 0b00, "j">;
  // def J.L
  def JX : GenericCallX<0b10, 0b10, "jx">;
}

def L8UI : LoadRRI8<0b0010, 0b0000, uimm8, "l8ui">;
def L16SI : LoadRRI8<0b0010, 0b1001, uimm8_sl1, "l16si">;
def L16UI : LoadRRI8<0b0010, 0b0001, uimm8_sl1, "l16ui">;
def L32I : LoadRRI8<0b0010, 0b0010, uimm8_sl2, "l32i">;
def L32IN : LoadRRRN<0b1000, uimm4_sl2, "l32i.n">;
// def L32R

def MEMW : I_Fixed24<0x0020c0, "memw", []>;

let isSelect = 1 in {
  def MOVEQZ : ArithRRR<0b0011, 0b1000, "moveqz">;
  def MOVNEZ : ArithRRR<0b0011, 0b1001, "movnez">;
  def MOVLTZ : ArithRRR<0b0011, 0b1010, "movltz">;
  def MOVGEZ : ArithRRR<0b0011, 0b1011, "movgez">;
}

def MOVI : ArithRI12<0b0010, 0b1010, movi_simm12, "movi">;
def MOVIN : ArithRI7N<0b1100, 0, movin_imm7, "movi.n">;

def MUL16S : ArithRRR<0b0001, 0b1101, "mul16s">;
def MUL16U : ArithRRR<0b0001, 0b1100, "mul16u">;

def MULL : ArithRRR<0b0010, 0b1000, "mull">;
def MULSH : ArithRRR<0b0010, 0b1011, "mulsh">;
def MULUH : ArithRRR<0b0010, 0b1010, "muluh">;

def NEG : ArithRRrt<0b0000, 0b0110, 0b0000, "neg">;

def NOP : I_Fixed24<0x0020f0, "nop", []>;
def NOPN : I_Fixed16<0xf03d, "nop.n", []>;

def OR : ArithRRR<0b0000, 0b0010, "or">;
def MOV : InstAlias<"mov $ar, $as", (OR GPR:$ar, GPR:$as, GPR:$as)>;

def QUOS : ArithRRR<0b0010, 0b1101, "quos">;
def QUOU : ArithRRR<0b0010, 0b1100, "quou">;
def REMS : ArithRRR<0b0010, 0b1111, "rems">;
def REMU : ArithRRR<0b0010, 0b1110, "remu">;

// def RER

let mayLoad = false, mayStore = false,
    isTerminator = true, isBarrier = true in {
  def RET : I_Fixed24<0x000080, "ret", []>;
  def RETN : I_Fixed16<0xf00d, "ret.n", []>;
  def RETW : I_Fixed24<0x000090, "retw", []>;
  def RETWN : I_Fixed16<0xf01d, "retw.n", []>;
}

// def RSR

def RSYNC : I_Fixed24<0x002010, "rsync", []>;

// def RUR

def S8I : StoreRRI8<0b0010, 0b0100, uimm8, "s8i">;
def S16I : StoreRRI8<0b0010, 0b0101, uimm8_sl1, "s16i">;
def S32I : StoreRRI8<0b0010, 0b0101, uimm8_sl2, "s32i">;
def S32IN : StoreRRRN<0b1001, uimm4_sl2, "s32i.n">;

// def S32NB

def SALT : ArithRRR<0b0010, 0b0111, "salt">;
def SALTU : ArithRRR<0b0010, 0b0110, "saltu">;

let Uses = [SAR] in {
  def SLL : ArithRRrs<0b0001, 0b1010, 0b0000, "sll">;
  def SRA : ArithRRrt<0b0001, 0b1011, 0b0000, "sra">;
  def SRC : ArithRRR<0b0001, 0b1000, "src">;
  def SRL : ArithRRrt<0b0001, 0b1001, 0b0000, "srl">;
}

let hasSideEffects = false, mayLoad = false, mayStore = false in {
  def SLLI : I_RRR<0b0000, 0b0001, 0b0000, (outs GPR:$ar), (ins GPR:$as, uimm5_sub32:$imm),
                   "slli $ar, $as, $imm", []> {
    bits<5> imm;

    let at = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRAI : I_RRR<0b0000, 0b0001, 0b0010, (outs GPR:$ar), (ins GPR:$at, uimm5:$imm),
                   "srai $ar, $at, $imm", []> {
    bits<5> imm;

    let as = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRLI : I_RRR<0b0000, 0b0001, 0b0100, (outs GPR:$ar), (ins GPR:$at, uimm4:$imm),
                   "srli $ar, $at, $imm", []> {
    bits<4> imm;

    let as = imm;
  }
}

let Defs = [SAR] in {
  def SSA8B : ArithRs<0b0000, 0b0100, 0b0000, 0b0011, "ssa8b">;
  def SSA8L : ArithRs<0b0000, 0b0100, 0b0000, 0b0010, "ssa8l">;
  def SSL : ArithRs<0b0000, 0b0100, 0b0000, 0b0001, "ssl">;
  def SSR : ArithRs<0b0000, 0b0100, 0b0000, 0b0000, "ssr">;

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

def SUB : ArithRRR<0b0000, 0b1100, "sub">;
def SUBX2 : ArithRRR<0b0000, 0b1101, "subx2">;
def SUBX4 : ArithRRR<0b0000, 0b1110, "subx4">;
def SUBX8 : ArithRRR<0b0000, 0b1111, "subx8">;

// def WER
// def WSR
// def WUR

def XOR : ArithRRR<0b0000, 0b0011, "xor">;

// def XSR
