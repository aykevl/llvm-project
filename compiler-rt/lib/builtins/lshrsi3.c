//===-- lshrsi3.c - Implement __lshrsi3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __lshrsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

typedef si_int lshr_int_t;
typedef uswords lshr_uwords_t;
#include "int_lshr_impl.inc"

// Returns: logical a >> b

// Precondition:  0 <= b < bits_in_sword

COMPILER_RT_ABI si_int __lshrsi3(si_int a, int b) {
  return __lshrXi3(a, b);
}
