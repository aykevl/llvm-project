//===-- ashlti3.c - Implement __ashlti3 -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __ashlti3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

#ifdef CRT_HAS_128BIT

typedef ti_int ashl_int_t;
typedef twords ashl_words_t;
#include "int_ashl_impl.inc"

// Returns: a << b

// Precondition:  0 <= b < bits_in_tword

COMPILER_RT_ABI ti_int __ashlti3(ti_int a, si_int b) {
  return __ashlXi3(a, b);
}

#endif // CRT_HAS_128BIT
