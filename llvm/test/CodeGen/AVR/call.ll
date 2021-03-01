; RUN: llc < %s -march=avr -mattr=avr6 | FileCheck %s

; TODO: test returning byval structs

declare i8 @foo8_1(i8)
declare i8 @foo8_2(i8, i8, i8)
declare i8 @foo8_3(i8, i8, i8, i8, i8, i8, i8, i8, i8, i8, i8)

declare i16 @foo16_1(i16, i16)
declare i16 @foo16_2(i16, i16, i16, i16, i16, i16, i16, i16, i16, i16, i16)

declare i32 @foo32_1(i32, i32)
declare i32 @foo32_2(i32, i32, i32, i32, i32)

declare i64 @foo64_1(i64)
declare i64 @foo64_2(i64, i64, i64)

define i8 @calli8_reg() {
; CHECK-LABEL: calli8_reg:
; CHECK: ldi r24, 12
; CHECK: call foo8_1
; CHECK: ldi r24, 12
; CHECK: ldi r22, 13
; CHECK: ldi r20, 14
; CHECK: call foo8_2
    %result1 = call i8 @foo8_1(i8 12)
    %result2 = call i8 @foo8_2(i8 12, i8 13, i8 14)
    ret i8 %result2
}

define i8 @calli8_stack() {
; CHECK-LABEL: calli8_stack:
; CHECK: ldi [[REG1:r[0-9]+]], 10
; CHECK: ldi [[REG2:r[0-9]+]], 11
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: call foo8_3
    %result1 = call i8 @foo8_3(i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 9, i8 10, i8 11)
    ret i8 %result1
}

define i16 @calli16_reg() {
; CHECK-LABEL: calli16_reg:
; CHECK: ldi r24, 1
; CHECK: ldi r25, 2
; CHECK: ldi r22, 2
; CHECK: ldi r23, 2
; CHECK: call foo16_1
    %result1 = call i16 @foo16_1(i16 513, i16 514)
    ret i16 %result1
}

define i16 @calli16_stack() {
; CHECK-LABEL: calli16_stack:
; CHECK: ldi [[REG1:r[0-9]+]], 10
; CHECK: ldi [[REG2:r[0-9]+]], 2
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 9
; CHECK: ldi [[REG2:r[0-9]+]], 2 
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: call foo16_2
    %result1 = call i16 @foo16_2(i16 512, i16 513, i16 514, i16 515, i16 516, i16 517, i16 518, i16 519, i16 520, i16 521, i16 522)
    ret i16 %result1
}

define i32 @calli32_reg() {
; CHECK-LABEL: calli32_reg:
; CHECK: ldi r22, 64
; CHECK: ldi r23, 66
; CHECK: ldi r24, 15
; CHECK: ldi r25, 2
; CHECK: ldi r18, 128
; CHECK: ldi r19, 132
; CHECK: ldi r20, 30
; CHECK: ldi r21, 2
; CHECK: call foo32_1
    %result1 = call i32 @foo32_1(i32 34554432, i32 35554432)
    ret i32 %result1
}

define i32 @calli32_stack() {
; CHECK-LABEL: calli32_stack:
; CHECK: ldi [[REG1:r[0-9]+]], 15
; CHECK: ldi [[REG2:r[0-9]+]], 2
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 64
; CHECK: ldi [[REG2:r[0-9]+]], 66
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: call foo32_2
    %result1 = call i32 @foo32_2(i32 1, i32 2, i32 3, i32 4, i32 34554432)
    ret i32 %result1
}

define i64 @calli64_reg() {
; CHECK-LABEL: calli64_reg:
; CHECK: ldi r18, 255
; CHECK: ldi r19, 255
; CHECK: ldi r20, 155
; CHECK: ldi r21, 88
; CHECK: ldi r22, 76
; CHECK: ldi r23, 73
; CHECK: ldi r24, 31
; CHECK: ldi r25, 242
; CHECK: call foo64_1
    %result1 = call i64 @foo64_1(i64 17446744073709551615)
    ret i64 %result1
}

define i64 @calli64_stack() {
; CHECK-LABEL: calli64_stack:

; CHECK: ldi [[REG1:r[0-9]+]], 31
; CHECK: ldi [[REG2:r[0-9]+]], 242
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 76
; CHECK: ldi [[REG2:r[0-9]+]], 73
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 155
; CHECK: ldi [[REG2:r[0-9]+]], 88
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 255
; CHECK: ldi [[REG2:r[0-9]+]], 255
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: call foo64_2
    %result1 = call i64 @foo64_2(i64 1, i64 2, i64 17446744073709551615)
    ret i64 %result1
}

; Test passing arguments through the stack when the call frame is allocated
; in the prologue.
declare void @foo64_3(i64, i64, i64, i8, i16*)

define void @testcallprologue() {
; CHECK-LABEL: testcallprologue:
; CHECK: push r28
; CHECK: push r29
; CHECK: sbiw r28, 27
; CHECK: ldi [[REG1:r[0-9]+]], 88
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 11
; CHECK: ldi [[REG2:r[0-9]+]], 10
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 13
; CHECK: ldi [[REG2:r[0-9]+]], 12
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 15
; CHECK: ldi [[REG2:r[0-9]+]], 14
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: ldi [[REG1:r[0-9]+]], 8
; CHECK: ldi [[REG2:r[0-9]+]], 9
; CHECK: push [[REG2]]
; CHECK: push [[REG1]]
; CHECK: pop r29
; CHECK: pop r28
  %p = alloca [8 x i16]
  %arraydecay = getelementptr inbounds [8 x i16], [8 x i16]* %p, i16 0, i16 0
  call void @foo64_3(i64 723685415333071112, i64 723685415333071112, i64 723685415333071112, i8 88, i16* %arraydecay)
  ret void
}

define i32 @icall(i32 (i32) addrspace(1)* %foo) {
; CHECK-LABEL: icall:
; CHECK: movw r30, r24
; CHECK: ldi r22, 147
; CHECK: ldi r23, 248
; CHECK: ldi r24, 214
; CHECK: ldi r25, 198
; CHECK: icall
; CHECK: subi r22, 251
; CHECK: sbci r23, 255
; CHECK: sbci r24, 255
; CHECK: sbci r25, 255
  %1 = call i32 %foo(i32 3335977107)
  %2 = add nsw i32 %1, 5
  ret i32 %2
}

; Calling external functions (like __divsf3) require extra processing for
; arguments and return values in the LowerCall function.
declare i32 @foofloat(float)

define i32 @externcall(float %a, float %b) {
; CHECK-LABEL: externcall:
; CHECK: movw [[REG1:(r[0-9]+|[XYZ])]], r24
; CHECK: movw [[REG2:(r[0-9]+|[XYZ])]], r22
; CHECK: movw r22, r18
; CHECK: movw r24, r20
; CHECK: movw r18, [[REG2]]
; CHECK: movw r20, [[REG1]]
; CHECK: call __divsf3
; CHECK: call foofloat
; CHECK: subi r22, 251
; CHECK: sbci r23, 255
; CHECK: sbci r24, 255
; CHECK: sbci r25, 255
  %1 = fdiv float %b, %a
  %2 = call i32 @foofloat(float %1)
  %3 = add nsw i32 %2, 5
  ret i32 %3
}

declare void @test_undef1_stack_target(i64, i64, i16, i16, i16, i32, i8) addrspace(1)

; test passing undef on the stack (by pushing r1)
define void @test_undef1_stack() addrspace(1) {
; CHECK-LABEL: test_undef1_stack:
; CHECK: ldi r24, 5
; CHECK: push r24
; CHECK: ldi r16, 0
; CHECK: ldi r17, 0
; CHECK: push r17
; CHECK: push r16
; CHECK: ldi r24, 3
; CHECK: ldi r25, 0
; CHECK: push r25
; CHECK: push r24
; CHECK: ldi r24, 32
; CHECK: ldi r25, 64
; CHECK: push r1
; CHECK: push r1
; CHECK: push r25
; CHECK: push r24
  tail call addrspace(1) void @test_undef1_stack_target(i64 0, i64 0, i16 0, i16 16416, i16 undef, i32 3, i8 5)
  ret void
}


declare void @test_undef2_stack_target(i64, i64, i16, i16) addrspace(1)

define void @test_undef2_stack() addrspace(1) {
; CHECK-LABEL: test_undef2_stack:
; CHECK: push r1
; CHECK: push r1
  tail call addrspace(1) void @test_undef2_stack_target(i64 0, i64 0, i16 0, i16 undef)
  ret void
}

declare void @test_undef3_stack_target(i64, i64, i16, i16, i8) addrspace(1)

define void @test_undef3_stack() addrspace(1) {
; CHECK-LABEL: test_undef3_stack:
; CHECK: ldi r24, 42
; CHECK: push r24
; CHECK: push r1
; CHECK: push r1
  tail call addrspace(1) void @test_undef3_stack_target(i64 0, i64 0, i16 0, i16 undef, i8 42)
  ret void
}
