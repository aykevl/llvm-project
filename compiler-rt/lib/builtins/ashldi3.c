// ====-- ashldi3.c - Implement __ashldi3 ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __ashldi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

typedef di_int ashl_int_t;
typedef dwords ashl_words_t;
#include "int_ashl_impl.inc"

// Returns: a << b

// Precondition:  0 <= b < bits_in_dword

COMPILER_RT_ABI di_int __ashldi3(di_int a, int b) {
  return __ashlXi3(a, b);
}

#if defined(__ARM_EABI__)
COMPILER_RT_ALIAS(__ashldi3, __aeabi_llsl)
#endif
