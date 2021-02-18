; RUN: llc < %s -march=avr | FileCheck %s

define i16 @zext1(i8 %x) {
; CHECK-LABEL: zext1:
; CHECK: mov r25, r1
  %1 = zext i8 %x to i16
  ret i16 %1
}

define i16 @zext2(i8 %x, i8 %y) {
; CHECK-LABEL: zext2:
; CHECK: mov r24, r22
; CHECK: mov r25, r1
  %1 = zext i8 %y to i16
  ret i16 %1
}

define i16 @zext_i1(i1 %x) {
; CHECK-LABEL: zext_i1:
; CHECK: andi r25, 0
  %1 = zext i1 %x to i16
  ret i16 %1
}

