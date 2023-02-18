// Integer comparison

// NOTE: Instcombine canonicalizes `icmp` with constants to have the constant
// on the left and avoid weak inequalities (`le`/`ge`), substantially reducing
// the number of patterns that need to be matched here. Other targets appear
// to do this as well.
def : Pat<(seteq i32:$a, 0),
          (MOVEQZ (MOVIN 0), (MOVIN 1), i32:$a)>;
def : Pat<(setne i32:$a, 0),
          (MOVEQZ (MOVIN 1), (MOVIN 0), i32:$a)>;
// NOTE: It is actually more profitable to let inequalities around zero be
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


// Conditional moves

// Generic fallback
def : Pat<(select i32:$cond, i32:$true, i32:$false),
          (MOVNEZ i32:$false, i32:$true, i32:$cond)>;
