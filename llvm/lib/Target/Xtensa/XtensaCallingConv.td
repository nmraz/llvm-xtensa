def RetCC_Xtensa_Call0 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCIfSplit<CCAssignToRegWithShadow<[A2, A4],
                                                    [A2, A3]>>>,
  CCIfType<[i32], CCAssignToReg<[A2, A3, A4, A5]>>,
]>;

def CC_Xtensa_Call0 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  // An i64 is split into two i32s, which must be passed in an even-based register pair.
  // When that happens, make sure the second half doesn't use the previous register.
  CCIfType<[i32], CCIfSplit<CCAssignToRegWithShadow<[A2, A4, A6],
                                                    [A2, A3, A5]>>>,
  // A split i64 placed on the stack must be aligned to an 8-byte boundary.
  // When that happens, make sure we don't attempt to use the remaining
  // odd-numbered register (if any).
  CCIfType<[i32], CCIfSplit<CCAssignToStackWithShadow<4, 8, [A7]>>>,
  CCIfType<[i32], CCAssignToReg<[A2, A3, A4, A5, A6, A7]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def CSR_Xtensa_Call0 : CalleeSavedRegs<(sequence "A%u", 12, 15)>;
