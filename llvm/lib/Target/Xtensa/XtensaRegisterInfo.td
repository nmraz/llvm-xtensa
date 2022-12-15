class XtensaReg<string n, bits<16> Enc, list<string> altNames = []> : Register<n, altNames> {
  let Namespace = "Xtensa";
  let HWEncoding = Enc;
}

def A0 : XtensaReg<"a0", 0>;
def A1 : XtensaReg<"a1", 1, ["sp"]>;
def A2 : XtensaReg<"a2", 2>;
def A3 : XtensaReg<"a3", 3>;
def A4 : XtensaReg<"a4", 4>;
def A5 : XtensaReg<"a5", 5>;
def A6 : XtensaReg<"a6", 6>;
def A7 : XtensaReg<"a7", 7>;
def A8 : XtensaReg<"a8", 8>;
def A9 : XtensaReg<"a9", 9>;
def A10 : XtensaReg<"a10", 10>;
def A11 : XtensaReg<"a11", 11>;
def A12 : XtensaReg<"a12", 12>;
def A13 : XtensaReg<"a13", 13>;
def A14 : XtensaReg<"a14", 14>;
def A15 : XtensaReg<"a15", 15>;

def SAR : XtensaReg<"sar", 3>;

def GPR : RegisterClass<"Xtensa", [i32], 32, (sequence "A%u", 0, 15)>;
