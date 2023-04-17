; RUN: llc < %s -mtriple=xtensa | FileCheck %s --check-prefix=OPT
; RUN: llc -O0 < %s -mtriple=xtensa | FileCheck %s --check-prefix=UNOPT

declare i32 @func(i32)

define i32 @test(i32 %a) {
; OPT-LABEL: test:
; OPT:       # %bb.0: # %entry
; OPT-NEXT:    addi a1, a1, -16
; OPT-NEXT:    movi.n a3, 14
; OPT-NEXT:    s32i.n a0, a1, 12 # 4-byte Spill
; OPT-NEXT:    bltu a3, a2, .LBB0_16
; OPT-NEXT:  # %bb.1: # %entry
; OPT-NEXT:    l32r a3, .LCPI0_0
; OPT-NEXT:    addx4 a2, a2, a3
; OPT-NEXT:    l32i.n a3, a2, 0
; OPT-NEXT:    movi.n a2, 5
; OPT-NEXT:    jx a3
; OPT-NEXT:  # %bb.2: # %sw.bb1
; OPT-NEXT:    movi.n a2, 17
; OPT-NEXT:    call0 func
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_16: # %sw.default
; OPT-NEXT:    movi.n a2, 8
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_3: # %sw.bb2
; OPT-NEXT:    movi.n a2, 28
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_4: # %sw.bb3
; OPT-NEXT:    movi a2, 553
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_5: # %sw.bb4
; OPT-NEXT:    movi a2, -92
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_6: # %sw.bb5
; OPT-NEXT:    movi.n a2, 7
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_7: # %sw.bb6
; OPT-NEXT:    movi a2, 100
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_8: # %sw.bb7
; OPT-NEXT:    movi.n a2, 90
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_9: # %sw.bb8
; OPT-NEXT:    movi.n a2, 18
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_10: # %sw.bb9
; OPT-NEXT:    movi.n a2, 20
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_11: # %sw.bb10
; OPT-NEXT:    movi a2, 700
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_12: # %sw.bb11
; OPT-NEXT:    movi a2, 666
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_13: # %sw.bb12
; OPT-NEXT:    movi a2, 144
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_14: # %sw.bb13
; OPT-NEXT:    movi.n a2, -1
; OPT-NEXT:    j .LBB0_17
; OPT-NEXT:  .LBB0_15: # %sw.bb14
; OPT-NEXT:    movi.n a2, 15
; OPT-NEXT:  .LBB0_17: # %return
; OPT-NEXT:    l32i.n a0, a1, 12 # 4-byte Reload
; OPT-NEXT:    addi a1, a1, 16
; OPT-NEXT:    ret.n
; OPT:       .LJTI0_0:
;
; UNOPT-LABEL: test:
; UNOPT:       # %bb.1: # %entry
; UNOPT-NEXT:    addi a1, a1, -16
; UNOPT-NEXT:    s32i.n a0, a1, 12 # 4-byte Spill
; UNOPT-NEXT:    mov.n a3, a2
; UNOPT-NEXT:    s32i.n a3, a1, 8 # 4-byte Spill
; UNOPT-NEXT:    movi.n a2, 14
; UNOPT-NEXT:    bltu a2, a3, .LBB0_16
; UNOPT-NEXT:  # %bb.18: # %entry
; UNOPT-NEXT:    l32i.n a2, a1, 8 # 4-byte Reload
; UNOPT-NEXT:    movi.n a3, 5
; UNOPT-NEXT:    l32r a4, .LCPI0_0
; UNOPT-NEXT:    addx4 a2, a2, a4
; UNOPT-NEXT:    l32i.n a2, a2, 0
; UNOPT-NEXT:    s32i.n a3, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    jx a2
; UNOPT-NEXT:  # %bb.2: # %sw.bb1
; UNOPT-NEXT:    movi.n a2, 17
; UNOPT-NEXT:    call0 func
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_3: # %sw.bb2
; UNOPT-NEXT:    movi.n a2, 28
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_4: # %sw.bb3
; UNOPT-NEXT:    movi a2, 553
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_5: # %sw.bb4
; UNOPT-NEXT:    movi a2, -92
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_6: # %sw.bb5
; UNOPT-NEXT:    movi.n a2, 7
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_7: # %sw.bb6
; UNOPT-NEXT:    movi a2, 100
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_8: # %sw.bb7
; UNOPT-NEXT:    movi.n a2, 90
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_9: # %sw.bb8
; UNOPT-NEXT:    movi.n a2, 18
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_10: # %sw.bb9
; UNOPT-NEXT:    movi.n a2, 20
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_11: # %sw.bb10
; UNOPT-NEXT:    movi a2, 700
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_12: # %sw.bb11
; UNOPT-NEXT:    movi a2, 666
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_13: # %sw.bb12
; UNOPT-NEXT:    movi a2, 144
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_14: # %sw.bb13
; UNOPT-NEXT:    movi.n a2, -1
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_15: # %sw.bb14
; UNOPT-NEXT:    movi.n a2, 15
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_16: # %sw.default
; UNOPT-NEXT:    movi.n a2, 8
; UNOPT-NEXT:    s32i.n a2, a1, 4 # 4-byte Spill
; UNOPT-NEXT:    j .LBB0_17
; UNOPT-NEXT:  .LBB0_17: # %return
; UNOPT-NEXT:    l32i.n a2, a1, 4 # 4-byte Reload
; UNOPT-NEXT:    l32i.n a0, a1, 12 # 4-byte Reload
; UNOPT-NEXT:    addi a1, a1, 16
; UNOPT-NEXT:    ret.n
; UNOPT:       .LJTI0_0:
entry:
  switch i32 %a, label %sw.default [
    i32 0, label %return
    i32 1, label %sw.bb1
    i32 2, label %sw.bb2
    i32 3, label %sw.bb3
    i32 4, label %sw.bb4
    i32 5, label %sw.bb5
    i32 6, label %sw.bb6
    i32 7, label %sw.bb7
    i32 8, label %sw.bb8
    i32 9, label %sw.bb9
    i32 10, label %sw.bb10
    i32 11, label %sw.bb11
    i32 12, label %sw.bb12
    i32 13, label %sw.bb13
    i32 14, label %sw.bb14
  ]

sw.bb1:
  %call = tail call i32 @func(i32 17)
  br label %return

sw.bb2:
  br label %return

sw.bb3:
  br label %return

sw.bb4:
  br label %return

sw.bb5:
  br label %return

sw.bb6:
  br label %return

sw.bb7:
  br label %return

sw.bb8:
  br label %return

sw.bb9:
  br label %return

sw.bb10:
  br label %return

sw.bb11:
  br label %return

sw.bb12:
  br label %return

sw.bb13:
  br label %return

sw.bb14:
  br label %return

sw.default:
  br label %return

return:
  %retval.0 = phi i32 [ 8, %sw.default ], [ 15, %sw.bb14 ], [ -1, %sw.bb13 ], [ 144, %sw.bb12 ], [ 666, %sw.bb11 ], [ 700, %sw.bb10 ], [ 20, %sw.bb9 ], [ 18, %sw.bb8 ], [ 90, %sw.bb7 ], [ 100, %sw.bb6 ], [ 7, %sw.bb5 ], [ -92, %sw.bb4 ], [ 553, %sw.bb3 ], [ 28, %sw.bb2 ], [ %call, %sw.bb1 ], [ 5, %entry ]
  ret i32 %retval.0
}
