class XtensaReg<string n, bits<16> Enc> : Register<n> {
  let Namespace = "Xtensa";
  let HWEncoding = Enc;
}

foreach Index = 0...15 in {
  def A#Index : XtensaReg<"a"#Index, Index>;
}

def SAR : XtensaReg<"sar", 3>;

def GPR : RegisterClass<"Xtensa", [i32], 32, (sequence "A%u", 0, 15)>;
