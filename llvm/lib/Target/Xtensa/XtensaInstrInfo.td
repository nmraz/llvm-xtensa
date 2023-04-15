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

def extui_lshrimm : ComplexPattern<i32, 2, "", []>;
def gi_extui_lshrimm : GIComplexOperandMatcher<s32, "selectExtuiLshrImm">,
                       GIComplexPatternEquiv<extui_lshrimm>;

def frameindexoff : ComplexPattern<iPTR, 2, "", []>;
def gi_frameindexoff : GIComplexOperandMatcher<s32, "selectFrameIndexOff">,
                       GIComplexPatternEquiv<frameindexoff>;

def imm_plus1_xform : SDNodeXForm<imm, [{ unimplemented }]>;
def gi_imm_plus1 : GICustomOperandRenderer<"renderImmPlus1">,
                   GISDNodeXFormEquiv<imm_plus1_xform>;

def b4const_minus1 : ImmLeaf<i32, [{
  return encodeB4Const(Imm + 1).has_value();
}], imm_plus1_xform>;

def b4constu_minus1 : ImmLeaf<i32, [{
  return encodeB4ConstU(Imm + 1).has_value();
}], imm_plus1_xform>;

// Codegen predicates

def HasMinMax : Predicate<"Subtarget->hasMinMax()">;
def HasSalt : Predicate<"Subtarget->hasSalt()">;

// Pseudo-instructions for codegen/lowering

let Uses = [A1], Defs = [A1] in {
  // Pseudo-instruction used to model allocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be reserved on the stack,
  // and `amt2` represents the number of bytes that have already been reserved
  // for the purposes of use in this call (beyond `amt1`).
  def ADJCALLSTACKDOWN : XtensaPseudo<(outs),
                                      (ins i32imm:$amt1, i32imm:$amt2), []>,
                                      Sched<[]>;

  // Pseudo-instruction used to model deallocation of stack slots for argument
  // passing. `amt1` represents the number of bytes to be popped off the stack,
  // and `amt2` represents the number of bytes that have already been popped by
  // the callee.
  def ADJCALLSTACKUP : XtensaPseudo<(outs),
                                    (ins i32imm:$amt1, i32imm:$amt2), []>,
                                    Sched<[]>;
}

// Pseudos expanded to conditional branches when the `saltu` instruction is not
// supported.
class XtensaSelectCCPseudo<DAGOperand CmpRHS>
   : XtensaPseudo<(outs GPR:$res),
                  (ins GPR:$lhs, CmpRHS:$rhs, GPR:$trueval, GPR:$falseval),
                  []> {
  let hasSideEffects = false;
  let usesCustomInserter = true;

  // These are immediately expanded into branches
  let hasNoSchedulingInfo = true;
}

def SELECT_LTU : XtensaSelectCCPseudo<GPR>;
def SELECT_LTUI : XtensaSelectCCPseudo<b4constu>;
def SELECT_GEUI : XtensaSelectCCPseudo<b4constu>;


// Real instructions

def ABS : ArithRRrtSched<0b0000, 0b0110, 0b0001, "abs",
                    [(set i32:$ar, (abs i32:$at))]>;

def ADD : ArithRRRSched<0b0000, 0b1000, "add", []>;
def ADDI : ArithRRI8<0b0010, 0b1100, simm8, "addi", []>;
def ADDIN : ArithRRI4N<0b1011, addin_imm4, "addi.n", []>;
def ADDMI : ArithRRI8<0b0010, 0b1101, simm8_sl8, "addmi", []>;

def ADDX2 : ArithRRRSched<0b0000, 0b1001, "addx2", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 1))))
]>;
def ADDX4 : ArithRRRSched<0b0000, 0b1010, "addx4", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 2))))
]>;
def ADDX8 : ArithRRRSched<0b0000, 0b1011, "addx8", [
  (set i32:$ar, (add i32:$at, (shl i32:$as, (i32 3))))
]>;

def ADDN : ArithRRRN<0b1010, "add.n", [(set i32:$ar, (add i32:$as, i32:$at))]>;

def AND : ArithRRRSched<0b0000, 0b0001, "and",
                        [(set i32:$ar, (and i32:$as, i32:$at))]>;

class XtensaRelaxedBranchPseudo<dag ins>
   : XtensaAsmPseudo<(outs), ins, []> {
  let Size = 6;
  let hasSideEffects = false;
  let mayLoad = false;
  let mayStore = false;
  let isBranch = true;
  let isTerminator = true;
}

multiclass RelaxableBranchRR8<bits<4> r, string name> {
  def NAME : BranchRR8<r, name>;
  def Relaxed : XtensaRelaxedBranchPseudo<(ins GPR:$as, GPR:$at,
                                           jmptarget18:$offset)>;
}
multiclass RelaxableBranchRB8<bits<3> arhi, string name> {
  def NAME : BranchRB8<arhi, name>;
  def Relaxed : XtensaRelaxedBranchPseudo<(ins GPR:$as, uimm5:$bbi,
                                           jmptarget18:$offset)>;
}
multiclass RelaxableBranchRI8<bits<2> m, string name> {
  def NAME: BranchRI8<m, name>;
  def Relaxed : XtensaRelaxedBranchPseudo<(ins GPR:$as, b4const:$imm,
                                           jmptarget18:$offset)>;
}
multiclass RelaxableBranchRUI8<bits<2> m, string name> {
  def NAME: BranchRUI8<m, name>;
  def Relaxed : XtensaRelaxedBranchPseudo<(ins GPR:$as, b4constu:$imm,
                                           jmptarget18:$offset)>;
}
multiclass RelaxableBranchRZ12<bits<2> m, string name> {
  def NAME: BranchRZ12<m, name>;
  def Relaxed : XtensaRelaxedBranchPseudo<(ins GPR:$as, jmptarget18:$offset)>;
}

defm BALL : RelaxableBranchRR8<0b0100, "ball">;
defm BANY : RelaxableBranchRR8<0b1000, "bany">;
defm BBC : RelaxableBranchRR8<0b0101, "bbc">;
defm BBCI : RelaxableBranchRB8<0b011, "bbci">;
defm BBS : RelaxableBranchRR8<0b1101, "bbs">;
defm BBSI : RelaxableBranchRB8<0b111, "bbsi">;
defm BEQ : RelaxableBranchRR8<0b0001, "beq">;
defm BEQI : RelaxableBranchRI8<0b00, "beqi">;
defm BEQZ : RelaxableBranchRZ12<0b00, "beqz">;
defm BGE : RelaxableBranchRR8<0b1010, "bge">;
defm BGEI : RelaxableBranchRI8<0b11, "bgei">;
defm BGEU : RelaxableBranchRR8<0b1011, "bgeu">;
defm BGEUI : RelaxableBranchRUI8<0b11, "bgeui">;
defm BGEZ : RelaxableBranchRZ12<0b11, "bgez">;
defm BLT : RelaxableBranchRR8<0b0010, "blt">;
defm BLTI : RelaxableBranchRI8<0b10, "blti">;
defm BLTU : RelaxableBranchRR8<0b0011, "bltu">;
defm BLTUI : RelaxableBranchRUI8<0b10, "bltui">;
defm BLTZ : RelaxableBranchRZ12<0b10, "bltz">;
defm BNALL : RelaxableBranchRR8<0b1100, "bnall">;
defm BNE : RelaxableBranchRR8<0b1001, "bne">;
defm BNEI : RelaxableBranchRI8<0b01, "bnei">;
defm BNEZ : RelaxableBranchRZ12<0b01, "bnez">;
defm BNONE : RelaxableBranchRR8<0b0000, "bnone">;

class CondBranchPat<dag cond, dag instr>
  : Pat<(brcond (i32 cond), bb:$offset), instr>;

// Bit testing branches

def : CondBranchPat<(and i32:$a, 1), (BBSI i32:$a, 0, bb:$offset)>;
def : CondBranchPat<(srl i32:$a, (i32 31)), (BLTZ i32:$a, bb:$offset)>;
def : CondBranchPat<(and (srl i32:$a, uimm5:$bit), 1),
                    (BBSI i32:$a, uimm5:$bit, bb:$offset)>;

def : CondBranchPat<(xor (and i32:$a, 1), 1),
                    (BBCI i32:$a, 0, bb:$offset)>;
def : CondBranchPat<(xor (srl i32:$a, (i32 31)), 1),
                    (BGEZ i32:$a, bb:$offset)>;
def : CondBranchPat<(xor (and (srl i32:$a, uimm5:$bit), 1), 1),
                    (BBCI i32:$a, uimm5:$bit, bb:$offset)>;

// Bitwise masking branches

def : CondBranchPat<(setne (and i32:$a, i32:$b), 0),
                    (BANY i32:$a, i32:$b, bb:$offset)>;
def : CondBranchPat<(seteq (and i32:$a, i32:$b), 0),
                    (BNONE i32:$a, i32:$b, bb:$offset)>;

// Arithmetic comparison branches

def : CondBranchPat<(seteq i32:$a, 0), (BEQZ i32:$a, bb:$offset)>;
def : CondBranchPat<(seteq i32:$a, i32:$b), (BEQ i32:$a, i32:$b, bb:$offset)>;
def : CondBranchPat<(setne i32:$a, 0), (BNEZ i32:$a, bb:$offset)>;
def : CondBranchPat<(setne i32:$a, i32:$b), (BNE i32:$a, i32:$b, bb:$offset)>;

def : CondBranchPat<(setgt i32:$a, -1), (BGEZ i32:$a, bb:$offset)>;
def : CondBranchPat<(setge i32:$a, 0), (BGEZ i32:$a, bb:$offset)>;
def : CondBranchPat<(setge i32:$a, i32:$b), (BGE i32:$a, i32:$b, bb:$offset)>;
def : CondBranchPat<(setgt i32:$a, i32:$b), (BLT i32:$b, i32:$a, bb:$offset)>;

def : CondBranchPat<(setle i32:$a, i32:$b), (BGE i32:$b, i32:$a, bb:$offset)>;
def : CondBranchPat<(setle i32:$a, -1), (BLTZ i32:$a, bb:$offset)>;
def : CondBranchPat<(setlt i32:$a, 0), (BLTZ i32:$a, bb:$offset)>;
def : CondBranchPat<(setlt i32:$a, i32:$b), (BLT i32:$a, i32:$b, bb:$offset)>;

def : CondBranchPat<(setuge i32:$a, i32:$b), (BGEU i32:$a, i32:$b, bb:$offset)>;
def : CondBranchPat<(setugt i32:$a, i32:$b), (BLTU i32:$b, i32:$a, bb:$offset)>;

def : CondBranchPat<(setule i32:$a, i32:$b), (BGEU i32:$b, i32:$a, bb:$offset)>;
def : CondBranchPat<(setult i32:$a, i32:$b), (BLTU i32:$a, i32:$b, bb:$offset)>;

// Arithmetic constant comparison branches

def : CondBranchPat<(seteq i32:$a, b4const:$imm),
                    (BEQI i32:$a, b4const:$imm, bb:$offset)>;
def : CondBranchPat<(setne i32:$a, b4const:$imm),
                    (BNEI i32:$a, b4const:$imm, bb:$offset)>;

def : CondBranchPat<(setge i32:$a, b4const:$imm),
                    (BGEI i32:$a, b4const:$imm, bb:$offset)>;
def : CondBranchPat<(setgt i32:$a, b4const_minus1:$imm),
                    (BGEI i32:$a, b4const_minus1:$imm, bb:$offset)>;

def : CondBranchPat<(setlt i32:$a, b4const:$imm),
                    (BLTI i32:$a, b4const:$imm, bb:$offset)>;
def : CondBranchPat<(setle i32:$a, b4const_minus1:$imm),
                    (BLTI i32:$a, b4const_minus1:$imm, bb:$offset)>;

def : CondBranchPat<(setuge i32:$a, b4constu:$imm),
                    (BGEUI i32:$a, b4constu:$imm, bb:$offset)>;
def : CondBranchPat<(setugt i32:$a, b4constu_minus1:$imm),
                    (BGEUI i32:$a, b4constu_minus1:$imm, bb:$offset)>;

def : CondBranchPat<(setult i32:$a, b4constu:$imm),
                    (BLTUI i32:$a, b4constu:$imm, bb:$offset)>;
def : CondBranchPat<(setule i32:$a, b4constu_minus1:$imm),
                    (BLTUI i32:$a, b4constu_minus1:$imm, bb:$offset)>;

// Fallback branch pattern

def : Pat<(brcond i32:$as, bb:$offset), (BNEZ i32:$as, bb:$offset)>;

def CALL0  : Call<0b00, A0, "call0">;
def CALL4  : Call<0b01, A4, "call4">;
def CALL8  : Call<0b10, A8, "call8">;
def CALL12 : Call<0b11, A12, "call12">;

def CALLX0  : CallX<0b00, A0, "callx0">;
def CALLX4  : CallX<0b01, A4, "callx4">;
def CALLX8  : CallX<0b10, A8, "callx8">;
def CALLX12 : CallX<0b11, A12, "callx12">;

// TODO: proper scheduling model
def DSYNC : I_Fixed24<0x002030, "dsync", []>, Sched<[]>;

def ENTRY : GenericBRI12<0b0110, 0b00, 0b11, uimm12_sl3, "entry">, Sched<[]> {
  // This essentially renames all registers.
  let hasSideEffects = true;
}

// TODO: proper scheduling model
def ESYNC : I_Fixed24<0x002020, "esync", []>, Sched<[]>;

// This instruction is so unusual that it's easiest to just define manually
def EXTUI
   : Inst24<0b0000, (outs GPR:$ar),
            (ins GPR:$at, uimm5:$shiftimm, uimm4_plus1:$maskimm),
            "extui $ar, $at, $shiftimm, $maskimm", []>,
            Sched<[WriteALU]> {
  let hasSideEffects = false;
  let mayLoad = false;
  let mayStore = false;
  let isReMaterializable = true;

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

def : Pat<(setlt i32:$a, 0),
          (EXTUI i32:$a, 31, 1)>;

// TODO: proper scheduling model
def EXTW : I_Fixed24<0x0020d0, "extw", []>, Sched<[]>;

let isTrap = true in {
  def ILL : I_Fixed24<0x000000, "ill", []>, Sched<[]>;
  def ILLN : I_Fixed16<0xf06d, "ill.n", []>, Sched<[]>;
}

// TODO: proper scheduling model
def ISYNC : I_Fixed24<0x002000, "isync", []>, Sched<[]>;

let isBranch = true, isTerminator = true, isBarrier = true in {
  def J : GenericCall<0b0110, 0b00, jmptarget18, "j", [(br bb:$offset)]>,
          Sched<[WriteBranch]>;
  def JX : GenericCallX<0b10, 0b10, "jx", []>, Sched<[WriteBranch]>;
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

def L32R : LoadRI16<0b0001, l32rtarget16, "l32r", []>, Sched<[WriteL32R]> {
  let isReMaterializable = true;
}

// TODO: proper scheduling model
def MEMW : I_Fixed24<0x0020c0, "memw", []>, Sched<[]>;

let Predicates = [HasMinMax] in {
  def MAX : ArithRRRSched<0b0011, 0b0101, "max",
                          [(set i32:$ar, (smax i32:$as, i32:$at))]>;
  def MAXU : ArithRRRSched<0b0011, 0b0111, "maxu",
                           [(set i32:$ar, (umax i32:$as, i32:$at))]>;
  def MIN : ArithRRRSched<0b0011, 0b0100, "min",
                          [(set i32:$ar, (smin i32:$as, i32:$at))]>;
  def MINU : ArithRRRSched<0b0011, 0b0110, "minu",
                           [(set i32:$ar, (umin i32:$as, i32:$at))]>;
}

let isSelect = true, isCommutable = true in {
  def MOVEQZ : MovRRR<0b1000, "moveqz", []>;
  def MOVNEZ : MovRRR<0b1001, "movnez", []>;
  def MOVLTZ : MovRRR<0b1010, "movltz", []>;
  def MOVGEZ : MovRRR<0b1011, "movgez", []>;
}

def : Pat<(select i32:$cond, i32:$true, i32:$false),
          (MOVEQZ i32:$true, i32:$false, i32:$cond)>;

let isAsCheapAsAMove = true, isReMaterializable = true, isMoveImm = true in {
  def MOVIN : ArithRI7N<0b1100, 0, movin_imm7, "movi.n", []>;
  def MOVI : ArithRI12<0b0010, 0b1010, simm12, "movi",
                       [(set i32:$at, simm12:$imm12)]>;
}

def MOVN : ArithBaseRRRN<0b1101, (outs GPR:$at), (ins GPR:$as),
                        "mov.n $at, $as", []>, Sched<[WriteALU]> {
  let isAsCheapAsAMove = true;
  let isMoveReg = true;

  let ar = 0b0000;
}

def MUL16S : ArithRRR<0b0001, 0b1101, "mul16s", []>, Sched<[WriteMul]>;
def MUL16U : ArithRRR<0b0001, 0b1100, "mul16u", []>, Sched<[WriteMul]>;

def MULL : ArithRRR<0b0010, 0b1000, "mull",
                           [(set i32:$ar, (mul i32:$as, i32:$at))]>,
                           Sched<[WriteMul]>;
def MULSH : ArithRRR<0b0010, 0b1011, "mulsh",
                            [(set i32:$ar, (mulhs i32:$as, i32:$at))]>,
                            Sched<[WriteMul]>;
def MULUH : ArithRRR<0b0010, 0b1010, "muluh",
                            [(set i32:$ar, (mulhu i32:$as, i32:$at))]>,
                            Sched<[WriteMul]>;

def NEG : ArithRRrtSched<0b0000, 0b0110, 0b0000, "neg",
                    [(set i32:$ar, (ineg i32:$at))]>;

def NOP : I_Fixed24<0x0020f0, "nop", []>, Sched<[]>;
def NOPN : I_Fixed16<0xf03d, "nop.n", []>, Sched<[]>;

def OR : ArithRRRSched<0b0000, 0b0010, "or",
                       [(set i32:$ar, (or i32:$as, i32:$at))]>;
def MOV : InstAlias<"mov $ar, $as", (OR GPR:$ar, GPR:$as, GPR:$as)>;

def QUOS : ArithRRR<0b0010, 0b1101, "quos",
                           [(set i32:$ar, (sdiv i32:$as, i32:$at))]>,
                           Sched<[WriteDiv]>;
def QUOU : ArithRRR<0b0010, 0b1100, "quou",
                           [(set i32:$ar, (udiv i32:$as, i32:$at))]>,
                           Sched<[WriteDiv]>;
def REMS : ArithRRR<0b0010, 0b1111, "rems",
                           [(set i32:$ar, (srem i32:$as, i32:$at))]>,
                           Sched<[WriteDiv]>;
def REMU : ArithRRR<0b0010, 0b1110, "remu",
                           [(set i32:$ar, (urem i32:$as, i32:$at))]>,
                           Sched<[WriteDiv]>;

// def RER

let Uses = [A0],
    isTerminator = true, isBarrier = true, isReturn = true,
    mayLoad = false, mayStore = false in {
  let hasSideEffects = false in {
    def RET : I_Fixed24<0x000080, "ret", []>, Sched<[WriteBranch]>;
    def RETN : I_Fixed16<0xf00d, "ret.n", []>, Sched<[WriteBranch]>;
  }

  // These instructions also have the side effect of incrementing the window
  // before performing the return.
  def RETW : I_Fixed24<0x000090, "retw", []>, Sched<[WriteBranch]>;
  def RETWN : I_Fixed16<0xf01d, "retw.n", []>, Sched<[WriteBranch]>;
}

// def RSR

// TODO: proper scheduling model
def RSYNC : I_Fixed24<0x002010, "rsync", []>, Sched<[]>;

// def RUR

def S8I : StoreRRI8<0b0010, 0b0100, uimm8, "s8i", []>;
def S16I : StoreRRI8<0b0010, 0b0101, uimm8_sl1, "s16i", []>;
def S32I : StoreRRI8<0b0010, 0b0110, uimm8_sl2, "s32i", []>;
def S32IN : StoreRRRN<0b1001, uimm4_sl2, "s32i.n", []>;

class FrameIndexOffStorePat<SDPatternOperator storeop, Instruction storeinst>
   : Pat<(storeop i32:$val, (frameindexoff i32:$fi, i32:$off)),
         (storeinst i32:$val, i32:$fi, i32:$off)>;

def : FrameIndexOffStorePat<truncstorei8, S8I>;
def : FrameIndexOffStorePat<truncstorei16, S16I>;
def : FrameIndexOffStorePat<store, S32I>;

// def S32NB

let Predicates = [HasSalt] in {
  def SALT : ArithRRRSched<0b0010, 0b0111, "salt", []>;
  def SALTU : ArithRRRSched<0b0010, 0b0110, "saltu", []>;
}

def SEXT
   : ArithBaseRRR<0b0011, 0b0010, (outs GPR:$ar),
                  (ins GPR:$as, uimm4_plus7:$imm),
                  "sext $ar, $as, $imm", []>, Sched<[WriteALU]> {
  bits<4> imm;
  let at = imm;
}

let Uses = [SAR] in {
  def SLL : ArithRRrs<0b0001, 0b1010, 0b0000, "sll", []>,
            Sched<[WriteALU, ReadDefault, ReadSAR]>;
  def SRA : ArithRRrt<0b0001, 0b1011, 0b0000, "sra", []>,
            Sched<[WriteALU, ReadDefault, ReadSAR]>;
  def SRC : ArithRRR<0b0001, 0b1000, "src", []>,
            Sched<[WriteALU, ReadDefault, ReadSAR]>;
  def SRL : ArithRRrt<0b0001, 0b1001, 0b0000, "srl", []>,
            Sched<[WriteALU, ReadDefault, ReadSAR]>;
}

let isReMaterializable = true in {
  def SLLI : ArithBaseRRR<0b0001, 0b0000, (outs GPR:$ar),
                          (ins GPR:$as, uimm5_sub32:$imm),
                          "slli $ar, $as, $imm",
                          [(set i32:$ar, (shl i32:$as, uimm5_sub32:$imm))]>,
                          Sched<[WriteALU]> {
    bits<5> imm;
    let at = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRAI : ArithBaseRRR<0b0001, 0b0010, (outs GPR:$ar),
                          (ins GPR:$at, uimm5:$imm),
                          "srai $ar, $at, $imm",
                          [(set i32:$ar, (sra i32:$at, uimm5:$imm))]>,
                          Sched<[WriteALU]> {
    bits<5> imm;
    let as = imm{3...0};
    let Inst{20} = imm{4};
  }

  def SRLI : ArithBaseRRR<0b0001, 0b0100, (outs GPR:$ar),
                          (ins GPR:$at, uimm4:$imm),
                          "srli $ar, $at, $imm",
                          [(set i32:$ar, (srl i32:$at, uimm4:$imm))]>,
                          Sched<[WriteALU]> {
    bits<4> imm;
    let as = imm;
  }
}

let Defs = [SAR] in {
  def SSA8B : ArithRs<0b0000, 0b0100, 0b0000, 0b0011, "ssa8b", []>,
              Sched<[ReadDefault, WriteSAR]>;
  def SSA8L : ArithRs<0b0000, 0b0100, 0b0000, 0b0010, "ssa8l", []>,
              Sched<[ReadDefault, WriteSAR]>;
  def SSL : ArithRs<0b0000, 0b0100, 0b0000, 0b0001, "ssl", []>,
            Sched<[ReadDefault, WriteSAR]>;
  def SSR : ArithRs<0b0000, 0b0100, 0b0000, 0b0000, "ssr", []>,
            Sched<[ReadDefault, WriteSAR]>;

  def SSAI : ArithBaseRRR<0b0000, 0b0100, (outs), (ins uimm5:$imm),
                          "ssai $imm", []>, Sched<[WriteSAR]> {
    bits<5> imm;

    let at = 0b0000;
    let at{0} = imm{4};
    let as = imm{3...0};
    let ar = 0b0100;
  }
}

def SUB : ArithRRRSched<0b0000, 0b1100, "sub",
                        [(set i32:$ar, (sub i32:$as, i32:$at))]>;

// Note: the generic selection of SSR/SSL is implemented manually in C++ due to
// a tablegen bug causing it to always add an extra implicit def.
def : Pat<(xtensa_ssr_masked uimm5:$a), (SSAI uimm5:$a)>;
def : Pat<(xtensa_ssr_masked (shl i32:$a, (i32 3))), (SSA8L i32:$a)>;
def : Pat<(xtensa_ssl_masked (shl i32:$a, (i32 3))), (SSA8B i32:$a)>;

def SUBX2 : ArithRRRSched<0b0000, 0b1101, "subx2", [
  (set i32:$ar, (sub (shl i32:$as, (i32 1)), i32:$at))
]>;
def SUBX4 : ArithRRRSched<0b0000, 0b1110, "subx4", [
  (set i32:$ar, (sub (shl i32:$as, (i32 2)), i32:$at))
]>;
def SUBX8 : ArithRRRSched<0b0000, 0b1111, "subx8", [
  (set i32:$ar, (sub (shl i32:$as, (i32 3)), i32:$at))
]>;

// def WER
// def WSR
// def WUR

def XOR : ArithRRRSched<0b0000, 0b0011, "xor",
                        [(set i32:$ar, (xor i32:$as, i32:$at))]>;

let Predicates = [HasSalt] in {
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
            (XOR (SALT i32:$a, i32:$b), (MOVI 1))>;
  def : Pat<(setle i32:$a, i32:$b),
            (XOR (SALT i32:$b, i32:$a), (MOVI 1))>;
  def : Pat<(setuge i32:$a, i32:$b),
            (XOR (SALTU i32:$a, i32:$b), (MOVI 1))>;
  def : Pat<(setule i32:$a, i32:$b),
            (XOR (SALTU i32:$b, i32:$a), (MOVI 1))>;
}

// def XSR
