//===-- lshrti3.c - Implement __lshrti3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __lshrti3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

#ifdef CRT_HAS_128BIT

typedef ti_int lshr_int_t;
typedef utwords lshr_uwords_t;
#include "int_lshr_impl.inc"

// Returns: logical a >> b

// Precondition:  0 <= b < bits_in_tword

COMPILER_RT_ABI ti_int __lshrti3(ti_int a, si_int b) {
  return __lshrXi3(a, b);
}

#endif // CRT_HAS_128BIT
