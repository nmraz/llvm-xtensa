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
  let Inst{0...3} = op0;
}

class Inst24<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : XtensaInst<outs, ins, asmstr, pattern> {
  field bits<24> Inst;

  let Size = 3;
  let Inst{0...3} = op0;
}

class I_RRR<bits<4> op0, bits<4> op1, bits<4> op2, dag outs, dag ins,
            string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> t;
  bits<4> s;
  bits<4> r;

  let Inst{16...19} = op1;
  let Inst{20...23} = op2;

  let Inst{4...7} = t;
  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_RRI4<bits<4> op0, bits<4> op1, dag outs, dag ins, string asmstr,
             list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> imm4;

  bits<4> t;
  bits<4> s;
  bits<4> r;

  let Inst{16...19} = op1;
  let Inst{20...23} = imm4;

  let Inst{4...7} = t;
  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_RRI8<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<8> imm8;

  bits<4> t;
  bits<4> s;
  bits<4> r;

  let Inst{16...23} = imm8;

  let Inst{4...7} = t;
  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_RI16<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<16> imm16;
  bits<4> t;

  let Inst{8...23} = imm16;
  let Inst{4...7} = t;
}

class I_RSR<bits<4> op0, bits<4> op1, bits<4> op2, dag outs, dag ins,
            string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> t;
  bits<8> rs;

  let Inst{16...19} = op1;
  let Inst{20...23} = op2;

  let Inst{4...7} = t;
  let Inst{8...15} = rs;
}

class I_CALL<bits<4> op0, bits<2> n, string asmstr, list<dag> pattern>
   : Inst24<op0, (outs), (ins), asmstr, pattern> {
  bits<18> offset;

  let Inst{4...5} = n;
  let Inst{6...23} = offset;
}

class I_CALLX<bits<4> op0, bits<4> op1, bits<4> op2, bits<2> m, bits<2> n,
              dag outs, dag ins, string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<4> r;
  bits<4> s;

  let Inst{16...19} = op1;
  let Inst{20...23} = op2;

  let Inst{4...5} = n;
  let Inst{6...7} = m;

  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_BRI8<bits<4> op0, bits<2> m, bits<2> n, dag outs, dag ins,
             string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<8> imm8;

  bits<4> r;
  bits<4> s;

  let Inst{16...23} = imm8;

  let Inst{4...5} = n;
  let Inst{6...7} = m;

  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_BRI12<bits<4> op0, bits<2> m, bits<2> n, dag outs, dag ins,
             string asmstr, list<dag> pattern>
   : Inst24<op0, outs, ins, asmstr, pattern> {
  bits<12> imm12;
  bits<4> s;

  let Inst{12...23} = imm12;

  let Inst{4...5} = n;
  let Inst{6...7} = m;

  let Inst{8...11} = s;
}

class I_RRRN<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<4> t;
  bits<4> s;
  bits<4> r;

  let Inst{4...7} = t;
  let Inst{8...11} = s;
  let Inst{12...15} = r;
}

class I_RI7<bits<4> op0, bit i, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<7> imm7;
  bits<4> s;

  let Inst{7} = i;
  let Inst{4...6} = imm7{4...6};
  let Inst{12...15} = imm7{0...3};
  let Inst{8...11} = s;
}

class I_RI6<bits<4> op0, bit i, bit z, dag outs, dag ins, string asmstr, list<dag> patterns>
   : Inst16<op0, outs, ins, asmstr, patterns> {
  bits<6> imm6;
  bits<4> s;

  let Inst{7} = i;
  let Inst{6} = z;
  let Inst{4...5} = imm6{4...5};
  let Inst{12...15} = imm6{0...3};
  let Inst{8...11} = s;
}
