// RUN: %clang_builtins %s %librt -o %t && %run %t
// REQUIRES: librt_has_ashrsi3
//===-- ashrsi3_test.c - Test __ashrsi3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file tests __ashrsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"
#include <stdio.h>

// Returns: arithmetic a >> b

// Precondition:  0 <= b < bits_in_dword

COMPILER_RT_ABI si_int __ashrsi3(si_int a, int b);

int test__ashrsi3(si_int a, int b, si_int expected) {
  si_int x = __ashrsi3(a, b);
  if (x != expected)
    printf("error in __ashrsi3: %X >> %d = %X, expected %X\n",
           a, b, __ashrsi3(a, b), expected);
  return x != expected;
}

int main() {
  if (test__ashrsi3(0x01234567, 0, 0x1234567))
    return 1;
  if (test__ashrsi3(0x01234567, 1, 0x91A2B3))
    return 1;
  if (test__ashrsi3(0x01234567, 2, 0x48D159))
    return 1;
  if (test__ashrsi3(0x01234567, 3, 0x2468AC))
    return 1;
  if (test__ashrsi3(0x01234567, 4, 0x123456))
    return 1;

  if (test__ashrsi3(0x01234567, 12, 0x1234))
    return 1;
  if (test__ashrsi3(0x01234567, 13, 0x91A))
    return 1;
  if (test__ashrsi3(0x01234567, 14, 0x48D))
    return 1;
  if (test__ashrsi3(0x01234567, 15, 0x246))
    return 1;

  if (test__ashrsi3(0x01234567, 16, 0x123))
    return 1;

  if (test__ashrsi3(0x01234567, 17, 0x91))
    return 1;
  if (test__ashrsi3(0x01234567, 18, 0x48))
    return 1;
  if (test__ashrsi3(0x01234567, 19, 0x24))
    return 1;
  if (test__ashrsi3(0x01234567, 20, 0x12))
    return 1;

  if (test__ashrsi3(0x01234567, 28, 0))
    return 1;
  if (test__ashrsi3(0x01234567, 29, 0))
    return 1;
  if (test__ashrsi3(0x01234567, 30, 0))
    return 1;
  if (test__ashrsi3(0x01234567, 31, 0))
    return 1;

  if (test__ashrsi3(0xFEDCBA98, 0, 0xFEDCBA98))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 1, 0xFF6E5D4C))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 2, 0xFFB72EA6))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 3, 0xFFDB9753))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 4, 0xFFEDCBA9))
    return 1;

  if (test__ashrsi3(0xFEDCBA98, 12, 0xFFFFEDCB))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 13, 0xFFFFF6E5))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 14, 0xFFFFFB72))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 15, 0xFFFFFDB9))
    return 1;

  if (test__ashrsi3(0xFEDCBA98, 16, 0xFFFFFEDC))
    return 1;

  if (test__ashrsi3(0xFEDCBA98, 17, 0xFFFFFF6E))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 18, 0xFFFFFFB7))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 19, 0xFFFFFFDB))
    return 1;
  if (test__ashrsi3(0xFEDCBA98, 20, 0xFFFFFFED))
    return 1;

  if (test__ashrsi3(0xAEDCBA98, 28, 0xFFFFFFFA))
    return 1;
  if (test__ashrsi3(0xAEDCBA98, 29, 0xFFFFFFFD))
    return 1;
  if (test__ashrsi3(0xAEDCBA98, 30, 0xFFFFFFFE))
    return 1;
  if (test__ashrsi3(0xAEDCBA98, 31, 0xFFFFFFFF))
    return 1;
  return 0;
}
