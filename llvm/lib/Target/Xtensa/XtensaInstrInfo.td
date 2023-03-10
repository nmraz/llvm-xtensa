include "XtensaInstrFormats.td"


// Pattern matching helpers

class SARUpdateGenericInstruction : XtensaGenericInstruction {
  let OutOperandList = (outs);
  let InOperandList = (ins type0:$amt);
  let Defs = [SAR];
  let hasSideEffects = false;
}

// Copy register to SAR, leaving an unspecified value if it is ugt 31
def G_XTENSA_SSR_INRANGE : SARUpdateGenericInstruction;
// Copy low 5 bits of register to SAR
def G_XTENSA_SSR_MASKED : SARUpdateGenericInstruction;
// Copy (32 - register) to SAR, leaving an unspecified value if it is ugt 31
def G_XTENSA_SSL_INRANGE : SARUpdateGenericInstruction;
// Copy (32 - (low 5 bits of register)) to SAR
def G_XTENSA_SSL_MASKED : SARUpdateGenericInstruction;

def SDTSetSAR : SDTypeProfile<0, 1, [SDTCisInt<0>]>;

def xtensa_ssr_masked : SDNode<"XtensaISD::SSR_MASKED", SDTSetSAR>;
def xtensa_ssl_masked : SDNode<"XtensaISD::SSL_MASKED", SDTSetSAR>;
def : GINodeEquiv<G_XTENSA_SSR_MASKED, xtensa_ssr_masked>;
def : GINodeEquiv<G_XTENSA_SSL_MASKED, xtensa_ssl_masked>;

def neg_imm_xform : SDNodeXForm<imm, [{ unimplemented }]>;
def gi_neg_imm : GICustomOperandRenderer<"renderNegImm">,
                 GISDNodeXFormEquiv<neg_imm_xform>;

def neg_simm8 : ImmLeaf<i32, [{ return isInt<8>(-Imm); }], neg_imm_xform>;
def neg_simm8_sl8 : ImmLeaf<i32, [{ return isShiftedInt<8, 8>(-Imm); }], neg_imm_xform>;

def extui_lshrimm : ComplexPattern<i32, 2, "", []>;
def gi_extui_lshrimm : GIComplexOperandMatcher<s32, "selectExtuiLshrImm">,
                       GIComplexPatternEquiv<extui_lshrimm>;

def frameindexoff : ComplexPattern<iPTR, 2, "", []>;
def gi_frameindexoff : GIComplexOperandMatcher<s32, "selectFrameIndexOff">,
                       GIComplexPatternEquiv<frameindexoff>;

// Pseudo-instructions for codegen/lowering

let Uses = [A1], Defs = [A1] in {
  // Pseudo-instruction used to model allocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be reserved on the stack,
  // and `amt2` represents the number of bytes that have already been reserved
  // for the purposes of use in this call (beyond `amt1`).
  def ADJCALLSTACKDOWN : XtensaPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), []>;

  // Pseudo-instruction used to model deallocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be popped off the stack,
  // and `amt2` represents the number of bytes that have already been popped by
  // the callee.
  def ADJCALLSTACKUP : XtensaPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), []>;
}


// Real instructions

def ABS : ArithRRrt<0b0000, 0b0110, 0b0001, "abs", []>;

def ADD   : ArithRRR<0b0000, 0b1000, "add", []>;

def ADDI  : ArithRRI8<0b0010, 0b1100, simm8, "addi", []>;
def ADDMI : ArithRRI8<0b0010, 0b1101, simm8_sl8, "addmi", []>;

def ADDX2 : ArithRRR<0b0000, 0b1001, "addx2", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 1))))
]>;
def ADDX4 : ArithRRR<0b0000, 0b1010, "addx4", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 2))))
]>;
def ADDX8 : ArithRRR<0b0000, 0b1011, "addx8", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 3))))
]>;

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

class FrameIndexOffLoadPat<SDPatternOperator loadop, Instruction loadinst>
   : Pat<(loadop (frameindexoff i32:$fi, i32:$off)),
         (loadinst i32:$fi, i32:$off)>;

// 8-bit loads
def : FrameIndexOffLoadPat<extloadi8, L8UI>;
def : FrameIndexOffLoadPat<zextloadi8, L8UI>;

// 16-bit loads
def : FrameIndexOffLoadPat<extloadi16, L16UI>;
def : FrameIndexOffLoadPat<zextloadi16, L16UI>;
def : FrameIndexOffLoadPat<sextloadi16, L16SI>;

// 32-bit loads
def : FrameIndexOffLoadPat<load, L32I>;

def L32R : LoadRI16<0b0001, l32rtarget16, "l32r", []>;

def MEMW : I_Fixed24<0x0020c0, "memw", []>;

let isSelect = true in {
  def MOVEQZ : MovRRR<0b1000, "moveqz", []>;
  def MOVNEZ : MovRRR<0b1001, "movnez", []>;
  def MOVLTZ : MovRRR<0b1010, "movltz", []>;
  def MOVGEZ : MovRRR<0b1011, "movgez", []>;
}

let isAsCheapAsAMove = true, isReMaterializable = true in {
  def MOVIN : ArithRI7N<0b1100, 0, movin_imm7, "movi.n", [(set i32:$as, movin_imm7:$imm7)]>;
  def MOVI : ArithRI12<0b0010, 0b1010, simm12, "movi", [(set i32:$at, simm12:$imm12)]>;

  def MOVN : ArithBaseRRRN<0b1101, (outs GPR:$at), (ins GPR:$as),
                          "mov.n $at, $as", []> {
    let ar = 0b0000;
  }
}

def MUL16S : ArithRRR<0b0001, 0b1101, "mul16s", []>;
def MUL16U : ArithRRR<0b0001, 0b1100, "mul16u", []>;

def MULL : ArithRRR<0b0010, 0b1000, "mull", [(set i32:$ar, (mul i32:$as, i32:$at))]>;
def MULSH : ArithRRR<0b0010, 0b1011, "mulsh", [(set i32:$ar, (mulhs i32:$as, i32:$at))]>;
def MULUH : ArithRRR<0b0010, 0b1010, "muluh", [(set i32:$ar, (mulhu i32:$as, i32:$at))]>;

def NEG : ArithRRrt<0b0000, 0b0110, 0b0000, "neg", [(set i32:$ar, (ineg i32:$at))]>;

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

class FrameIndexOffStorePat<SDPatternOperator storeop, Instruction storeinst>
   : Pat<(storeop i32:$val, (frameindexoff i32:$fi, i32:$off)),
         (storeinst i32:$val, i32:$fi, i32:$off)>;

def : FrameIndexOffStorePat<truncstorei8, S8I>;
def : FrameIndexOffStorePat<truncstorei16, S16I>;
def : FrameIndexOffStorePat<store, S32I>;

// def S32NB

def SALT : ArithRRR<0b0010, 0b0111, "salt", []>;
def SALTU : ArithRRR<0b0010, 0b0110, "saltu", []>;

let Uses = [SAR] in {
  def SLL : ArithRRrs<0b0001, 0b1010, 0b0000, "sll", []>;
  def SRA : ArithRRrt<0b0001, 0b1011, 0b0000, "sra", []>;
  def SRC : ArithRRR<0b0001, 0b1000, "src", []>;
  def SRL : ArithRRrt<0b0001, 0b1001, 0b0000, "srl", []>;
}

def SLLI : ArithBaseRRR<0b0001, 0b0000, (outs GPR:$ar), (ins GPR:$as, uimm5_sub32:$imm),
                  "slli $ar, $as, $imm", [(set i32:$ar, (shl i32:$as, uimm5_sub32:$imm))]> {
  bits<5> imm;
  let at = imm{3...0};
  let Inst{20} = imm{4};
}

def SRAI : ArithBaseRRR<0b0001, 0b0010, (outs GPR:$ar), (ins GPR:$at, uimm5:$imm),
                  "srai $ar, $at, $imm", [(set i32:$ar, (sra i32:$at, uimm5:$imm))]> {
  bits<5> imm;
  let as = imm{3...0};
  let Inst{20} = imm{4};
}

def SRLI : ArithBaseRRR<0b0001, 0b0100, (outs GPR:$ar), (ins GPR:$at, uimm4:$imm),
                  "srli $ar, $at, $imm", [(set i32:$ar, (srl i32:$at, uimm4:$imm))]> {
  bits<4> imm;
  let as = imm;
}

let Defs = [SAR] in {
  def SSA8B : ArithRs<0b0000, 0b0100, 0b0000, 0b0011, "ssa8b", []>;
  def SSA8L : ArithRs<0b0000, 0b0100, 0b0000, 0b0010, "ssa8l", []>;
  def SSL : ArithRs<0b0000, 0b0100, 0b0000, 0b0001, "ssl", []>;
  def SSR : ArithRs<0b0000, 0b0100, 0b0000, 0b0000, "ssr", []>;

  def SSAI : ArithBaseRRR<0b0000, 0b0100, (outs), (ins uimm5:$imm),
                  "ssai $imm", []> {
    bits<5> imm;

    let at = 0b0000;
    let at{0} = imm{4};
    let as = imm{3...0};
    let ar = 0b0100;
  }
}

def SUB : ArithRRR<0b0000, 0b1100, "sub", [(set i32:$ar, (sub i32:$as, i32:$at))]>;

// Note: the generic selection of SSR/SSL is implemented manually in C++ due to
// a tablegen bug causing it to always add an extra implicit def.
def : Pat<(xtensa_ssr_masked uimm5:$a), (SSAI uimm5:$a)>;
def : Pat<(xtensa_ssr_masked (shl i32:$a, (i32 3))), (SSA8L i32:$a)>;
def : Pat<(xtensa_ssl_masked (shl i32:$a, (i32 3))), (SSA8B i32:$a)>;

def SUBX2 : ArithRRR<0b0000, 0b1101, "subx2", [
  (set i32:$ar, (sub (shl i32:$as, (i32 1)), i32:$at))
]>;
def SUBX4 : ArithRRR<0b0000, 0b1110, "subx4", [
  (set i32:$ar, (sub (shl i32:$as, (i32 2)), i32:$at))
]>;
def SUBX8 : ArithRRR<0b0000, 0b1111, "subx8", [
  (set i32:$ar, (sub (shl i32:$as, (i32 3)), i32:$at))
]>;

// def WER
// def WSR
// def WUR

def XOR : ArithRRR<0b0000, 0b0011, "xor", [(set i32:$ar, (xor i32:$as, i32:$at))]>;

// def XSR

// Integer comparison

// NOTE: Instcombine canonicalizes `icmp` with constants to have the constant
// on the left and avoid weak inequalities (`le`/`ge`), substantially reducing
// the number of patterns that need to be matched here. Other targets appear
// to do this as well.
def : Pat<(seteq i32:$a, 0),
          (MOVEQZ (MOVIN 0), (MOVIN 1), i32:$a)>;
def : Pat<(setne i32:$a, 0),
          (MOVEQZ (MOVIN 1), (MOVIN 0), i32:$a)>;
def : Pat<(setlt i32:$a, 0),
          (EXTUI i32:$a, 31, 1)>;
// NOTE: It is actually more profitable to let other inequalities around zero be
// selected to a MOVI + SALT[U] instead of trying to use two MOVI's and a
// MOV*Z.

def : Pat<(seteq i32:$a, i32:$b),
          (MOVEQZ (MOVIN 0), (MOVIN 1), (SUB i32:$a, i32:$b))>;
def : Pat<(setne i32:$a, i32:$b),
          (MOVEQZ (MOVIN 1), (MOVIN 0), (SUB i32:$a, i32:$b))>;

def : Pat<(setlt i32:$a, i32:$b),
          (SALT i32:$a, i32:$b)>;
def : Pat<(setgt i32:$a, i32:$b),
          (SALT i32:$b, i32:$a)>;
def : Pat<(setult i32:$a, i32:$b),
          (SALTU i32:$a, i32:$b)>;
def : Pat<(setugt i32:$a, i32:$b),
          (SALTU i32:$b, i32:$a)>;

// These patterns generally only show up in unoptimized code
def : Pat<(setge i32:$a, i32:$b),
          (XOR (SALT i32:$a, i32:$b), (MOVIN 1))>;
def : Pat<(setle i32:$a, i32:$b),
          (XOR (SALT i32:$b, i32:$a), (MOVIN 1))>;
def : Pat<(setuge i32:$a, i32:$b),
          (XOR (SALTU i32:$a, i32:$b), (MOVIN 1))>;
def : Pat<(setule i32:$a, i32:$b),
          (XOR (SALTU i32:$b, i32:$a), (MOVIN 1))>;


// Conditional moves

// Once more, we leave generalized comparisons to be selected through the
// patterns above and the generic fallback below. This is assisted by the fact
// that instcombine favors strong comparisons, ensuring there are no redundant
// XOR's.
def : Pat<(select (i32 (seteq i32:$val, 0)), i32:$true, i32:$false),
          (MOVEQZ i32:$false, i32:$true, i32:$val)>;
def : Pat<(select (i32 (setne i32:$val, 0)), i32:$true, i32:$false),
          (MOVEQZ i32:$true, i32:$false, i32:$val)>;
def : Pat<(select (i32 (setlt i32:$val, 0)), i32:$true, i32:$false),
          (MOVGEZ i32:$true, i32:$false, i32:$val)>;
def : Pat<(select (i32 (setlt i32:$val, 1)), i32:$true, i32:$false),
          (MOVGEZ i32:$false, i32:$true, (NEG i32:$val))>;
def : Pat<(select (i32 (setgt i32:$val, 0)), i32:$true, i32:$false),
          (MOVGEZ i32:$true, i32:$false, (NEG i32:$val))>;

def : Pat<(select (i32 (seteq i32:$val1, i32:$val2)), i32:$true, i32:$false),
          (MOVEQZ i32:$false, i32:$true, (SUB i32:$val1, i32:$val2))>;

// Generic fallback
def : Pat<(select i32:$cond, i32:$true, i32:$false),
          (MOVEQZ i32:$true, i32:$false, i32:$cond)>;
