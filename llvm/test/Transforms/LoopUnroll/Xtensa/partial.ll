; RUN: opt < %s -S -loop-unroll | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "xtensa"

define dso_local void @xor_loop(ptr nocapture noundef %a, ptr nocapture noundef readonly %b, i32 noundef %n) local_unnamed_addr {
entry:
  %cmp8.not = icmp eq i32 %n, 0
  br i1 %cmp8.not, label %for.cond.cleanup, label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  ret void

for.body:                                         ; preds = %entry, %for.body
  %i.09 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %arrayidx = getelementptr inbounds i8, ptr %b, i32 %i.09
  %0 = load i8, ptr %arrayidx, align 1
  %arrayidx1 = getelementptr inbounds i8, ptr %a, i32 %i.09
  %1 = load i8, ptr %arrayidx1, align 1
  %xor7 = xor i8 %1, %0
  store i8 %xor7, ptr %arrayidx1, align 1
  %inc = add nuw i32 %i.09, 1
  %exitcond.not = icmp eq i32 %inc, %n
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body

; CHECK-LABEL: @xor_loop
; CHECK: for.body:
; CHECK: xor i8
; CHECK-NEXT: store
; CHECK: xor i8
; CHECK-NEXT: store
; CHECK: xor i8
; CHECK-NEXT: store
; CHECK: xor i8
; CHECK-NEXT: store
; CHECK-NOT: xor i8
; CHECK: br
}
