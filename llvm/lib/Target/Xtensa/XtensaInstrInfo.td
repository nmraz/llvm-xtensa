include "XtensaInstrFormats.td"


// Pattern matching helpers
def extui_lshrimm : ComplexPattern<i32, 2, "", []>;
def gi_extui_lshrimm : GIComplexOperandMatcher<s32, "selectExtuiLshrImm">,
                       GIComplexPatternEquiv<extui_lshrimm>;


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
def BEQZ : BranchRZ12<0b00, "beqz", []>;
def BGE : BranchRR8<0b1010, "bge">;
def BGEI : BranchRI8<0b11, "bgei">;
def BGEU : BranchRR8<0b1011, "bgeu">;
def BGEUI : BranchRUI8<0b11, "bgeui">;
def BGEZ : BranchRZ12<0b11, "bgez", []>;
def BLT : BranchRR8<0b0010, "blt">;
def BLTI : BranchRI8<0b10, "blti">;
def BLTU : BranchRR8<0b0011, "bltu">;
def BLTUI : BranchRUI8<0b10, "bltui">;
def BLTZ : BranchRZ12<0b10, "bltz", []>;
def BNALL : BranchRR8<0b1100, "bnall">;
def BNE : BranchRR8<0b1001, "bne">;
def BNEI : BranchRI8<0b01, "bnei">;
def BNEZ : BranchRZ12<0b01, "bnez", [(brcond i32:$as, bb:$offset)]>;
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

def ENTRY : GenericBRI12<0b0110, 0b00, 0b11, uimm12_sl3, "entry", []> {
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
  def J : GenericCall<0b0110, 0b00, jmptarget18, "j", [(br bb:$offset)]>;
  // def J.L
  def JX : GenericCallX<0b10, 0b10, "jx", []>;
}

def L8UI : LoadRRI8<0b0010, 0b0000, uimm8, "l8ui", []>;
def L16SI : LoadRRI8<0b0010, 0b1001, uimm8_sl1, "l16si", []>;
def L16UI : LoadRRI8<0b0010, 0b0001, uimm8_sl1, "l16ui", []>;
def L32I : LoadRRI8<0b0010, 0b0010, uimm8_sl2, "l32i", []>;
def L32IN : LoadRRRN<0b1000, uimm4_sl2, "l32i.n", []>;
def L32R : LoadRI16<0b0001, l32rtarget16, "l32r", []>;

def MEMW : I_Fixed24<0x0020c0, "memw", []>;

let isSelect = 1 in {
  def MOVEQZ : MovRRR<0b1000, "moveqz", []>;
  def MOVNEZ : MovRRR<0b1001, "movnez", []>;
  def MOVLTZ : MovRRR<0b1010, "movltz", []>;
  def MOVGEZ : MovRRR<0b1011, "movgez", []>;
}

def : Pat<(select i32:$cond, i32:$true, i32:$false),
          (MOVNEZ i32:$false, i32:$true, i32:$cond)>;

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

def SALT : ArithRRR<0b0010, 0b0111, "salt", []>;
def SALTU : ArithRRR<0b0010, 0b0110, "saltu", []>;

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

def : Pat<(seteq i32:$a, i32:$b),
          (MOVEQZ (MOVIN 0), (MOVIN 1), (SUB i32:$a, i32:$b))>;
def : Pat<(setne i32:$a, i32:$b),
          (MOVEQZ (MOVIN 1), (MOVIN 0), (SUB i32:$a, i32:$b))>;

def : Pat<(setlt i32:$a, i32:$b),
          (SALT i32:$a, i32:$b)>;
def : Pat<(setgt i32:$a, i32:$b),
          (SALT i32:$b, i32:$a)>;
def : Pat<(setge i32:$a, i32:$b),
          (XOR (SALT i32:$a, i32:$b), (MOVIN 1))>;
def : Pat<(setle i32:$a, i32:$b),
          (XOR (SALT i32:$b, i32:$a), (MOVIN 1))>;

def : Pat<(setult i32:$a, i32:$b),
          (SALTU i32:$a, i32:$b)>;
def : Pat<(setugt i32:$a, i32:$b),
          (SALTU i32:$b, i32:$a)>;
def : Pat<(setuge i32:$a, i32:$b),
          (XOR (SALTU i32:$a, i32:$b), (MOVIN 1))>;
def : Pat<(setule i32:$a, i32:$b),
          (XOR (SALTU i32:$b, i32:$a), (MOVIN 1))>;
