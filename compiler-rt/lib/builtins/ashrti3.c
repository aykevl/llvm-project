//===-- ashrti3.c - Implement __ashrti3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __ashrti3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

#ifdef CRT_HAS_128BIT

typedef ti_int ashr_int_t;
typedef twords ashr_words_t;
#include "int_ashr_impl.inc"

// Returns: arithmetic a >> b

// Precondition:  0 <= b < bits_in_tword

COMPILER_RT_ABI ti_int __ashrti3(ti_int a, si_int b) {
  return __ashrXi3(a, b);
}

#endif // CRT_HAS_128BIT
