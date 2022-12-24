def RetCC_Xtensa_Call0 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[A2]>>,
  CCIfType<[i64], CCAssignToRegWithShadow<[A2], [A3]>>
]>;

def CC_Xtensa_Call0 : CallingConv<[
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,
  CCIfType<[i32], CCAssignToReg<[A2, A3, A4, A5, A6, A7]>>,
  CCIfType<[i64], CCAssignToRegWithShadow<[A2, A4, A6], [A3, A5, A7]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>,
  CCIfType<[i64], CCAssignToStack<8, 8>>
]>;
