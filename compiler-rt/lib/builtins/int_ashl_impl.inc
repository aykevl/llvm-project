//===-- int_ashl_impl.inc - Integer bitshifts -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Helpers used by __ashlsi3, __ashldi3, and __ashlti3.
//
//===----------------------------------------------------------------------===//

static __inline ashl_int_t __ashlXi3(ashl_int_t a, int b) {
  const int bits_in_word = (int)(sizeof(ashl_int_t) * CHAR_BIT) / 2;
  ashl_words_t input;
  ashl_words_t result;
  input.all = a;
  if (b & bits_in_word) {
    // bits_in_word <= b < bits_in_sword
    result.s.low = 0;
    result.s.high = input.s.low << (b - bits_in_word);
  } else {
    // 0 <= b < bits_in_word
    if (b == 0)
      return a;
    result.s.low = input.s.low << b;
    result.s.high = (input.s.high << b) | (input.s.low >> (bits_in_word - b));
  }
  return result.all;
}
