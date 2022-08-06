include "XtensaInstrFormats.td"

def Test : Inst16<0b0101, (outs GPR:$out), (ins GPR:$in), "test $out, $in", []>;
