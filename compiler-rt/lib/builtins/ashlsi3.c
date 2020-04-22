// ====-- ashlsi3.c - Implement __ashlsi3 ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __ashlsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

typedef si_int ashl_int_t;
typedef swords ashl_words_t;
#include "int_ashl_impl.inc"

// Returns: a << b

// Preconsition:  0 <= b < bits_in_sword

COMPILER_RT_ABI si_int __ashlsi3(si_int a, int b) {
  return __ashlXi3(a, b);
}
