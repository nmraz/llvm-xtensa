class XtensaInst<dag outs, dag ins, string asmstr, list<dag> pattern>
   : Instruction {
  let Namespace = "Xtensa";

  let OutOperandList = outs;
  let InOperandList  = ins;

  let AsmString   = asmstr;
  let Pattern     = pattern;
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
