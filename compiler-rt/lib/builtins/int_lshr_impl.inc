//===-- int_lshr_impl.inc - Integer bitshifts -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Helpers used by __lshrsi3, __lshrdi3, and __lshrti3.
//
//===----------------------------------------------------------------------===//

static __inline lshr_int_t __lshrXi3(lshr_int_t a, int b) {
  const int bits_in_word = (int)(sizeof(lshr_int_t) * CHAR_BIT) / 2;
  lshr_uwords_t input;
  lshr_uwords_t result;
  input.all = a;
  if (b & bits_in_word) {
    // bits_in_word <= b < bits_in_sword
    result.s.high = 0;
    result.s.low = input.s.high >> (b - bits_in_word);
  } else {
    // 0 <= b < bits_in_word
    if (b == 0)
      return a;
    result.s.high = input.s.high >> b;
    result.s.low = (input.s.high << (bits_in_word - b)) | (input.s.low >> b);
  }
  return result.all;
}
