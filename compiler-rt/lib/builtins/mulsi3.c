//===-- mulsi3.c - Implement __mulsi3 -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __mulsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

// Based on muldi3.c

// Returns: a * b

static si_int __mulshi3(hu_int a, hu_int b) {
  swords r;
  const int bits_in_word_2 = (int)(sizeof(hi_int) * CHAR_BIT) / 2;
  const hu_int lower_mask = (hu_int)~0 >> bits_in_word_2;
  r.s.low = (a & lower_mask) * (b & lower_mask);
  hu_int t = r.s.low >> bits_in_word_2;
  r.s.low &= lower_mask;
  t += (a >> bits_in_word_2) * (b & lower_mask);
  r.s.low += (t & lower_mask) << bits_in_word_2;
  r.s.high = t >> bits_in_word_2;
  t = r.s.low >> bits_in_word_2;
  r.s.low &= lower_mask;
  t += (b >> bits_in_word_2) * (a & lower_mask);
  r.s.low += (t & lower_mask) << bits_in_word_2;
  r.s.high += t >> bits_in_word_2;
  r.s.high += (a >> bits_in_word_2) * (b >> bits_in_word_2);
  return r.all;
}

// Returns: a * b

COMPILER_RT_ABI si_int __mulsi3(si_int a, si_int b) {
  swords x;
  x.all = a;
  swords y;
  y.all = b;
  swords r;
  r.all = __mulshi3(x.s.low, y.s.low);
  r.s.high += x.s.high * y.s.low + x.s.low * y.s.high;
  return r.all;
}
