// RUN: %clang_builtins %s %librt -o %t && %run %t
// REQUIRES: librt_has_ashlsi3
//===-- ashlsi3_test.c - Test __ashlsi3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file tests __ashlsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"
#include <stdio.h>

// Returns: a << b

// Precondition:  0 <= b < bits_in_dword

COMPILER_RT_ABI si_int __ashlsi3(si_int a, int b);

int test__ashlsi3(si_int a, int b, si_int expected) {
  si_int x = __ashlsi3(a, b);
  if (x != expected)
    printf("error in __ashlsi3: %X << %d = %X, expected %X\n",
           a, b, __ashlsi3(a, b), expected);
  return x != expected;
}

int main() {
  if (test__ashlsi3(0x01234567, 0, 0x1234567))
    return 1;
  if (test__ashlsi3(0x01234567, 1, 0x2468ACE))
    return 1;
  if (test__ashlsi3(0x01234567, 2, 0x048D159C))
    return 1;
  if (test__ashlsi3(0x01234567, 3, 0x091A2B38))
    return 1;
  if (test__ashlsi3(0x01234567, 4, 0x12345670))
    return 1;

  if (test__ashlsi3(0x01234567, 12, 0x34567000))
    return 1;
  if (test__ashlsi3(0x01234567, 13, 0x68ACE000))
    return 1;
  if (test__ashlsi3(0x01234567, 14, 0xD159C000))
    return 1;
  if (test__ashlsi3(0x01234567, 15, 0xA2B38000))
    return 1;

  if (test__ashlsi3(0x01234567, 16, 0x45670000))
    return 1;

  if (test__ashlsi3(0x01234567, 17, 0x8ACE0000))
    return 1;
  if (test__ashlsi3(0x01234567, 18, 0x159C0000))
    return 1;
  if (test__ashlsi3(0x01234567, 19, 0x2B380000))
    return 1;
  if (test__ashlsi3(0x01234567, 20, 0x56700000))
    return 1;

  if (test__ashlsi3(0x01234567, 28, 0x70000000))
    return 1;
  if (test__ashlsi3(0x01234567, 29, 0xE0000000))
    return 1;
  if (test__ashlsi3(0x01234567, 30, 0xC0000000))
    return 1;
  if (test__ashlsi3(0x01234567, 31, 0x80000000))
    return 1;
  return 0;
}
