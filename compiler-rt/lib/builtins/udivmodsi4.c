//===-- udivmodsi4.c - Implement __udivmodsi4 -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements __udivmodsi4 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"

// Returns: a / b, *rem = a % b

COMPILER_RT_ABI du_int __udivmodsi4(su_int a, su_int b) {
  si_int d = __udivsi3(a, b);
  si_int rem = a - (d * b);
  return (du_int)d | ((du_int)rem << 32);
}
