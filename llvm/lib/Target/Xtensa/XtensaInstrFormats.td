class XtensaInst<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : Instruction {
  let Namespace = "Xtensa";

  bits<4> Op0 = op0;

  let OutOperandList = outs;
  let InOperandList  = ins;

  let AsmString   = asmstr;
  let Pattern     = pattern;
}

class Inst16<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : XtensaInst<op0, outs, ins, asmstr, pattern> {
  field bits<16> Inst;

  let Size = 2;
  let Inst{0...3} = Op0;
}

class Inst24<bits<4> op0, dag outs, dag ins, string asmstr, list<dag> pattern>
   : XtensaInst<op0, outs, ins, asmstr, pattern> {
  field bits<24> Inst;

  let Size = 3;
  let Inst{0...3} = Op0;
}
