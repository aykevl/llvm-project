// RUN: %clang_builtins %s %librt -o %t && %run %t
// REQUIRES: librt_has_divmodsi4
//===-- divmodsi4_test.c - Test __divmodsi4 -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file tests __divmodsi4 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"
#include <stdio.h>

// Returns: a / b

extern COMPILER_RT_ABI du_int __divmodsi4(si_int a, si_int b);


int test__divmodsi4(si_int a, si_int b, 
						si_int expected_result, si_int expected_rem)
{
    du_int pair = __divmodsi4(a, b);
    si_int result = (su_int)pair;
    si_int rem = (su_int)(pair >> 32);
    if (result != expected_result) {
        printf("error in __divmodsi4: %d / %d = %d, expected %d\n",
               a, b, result, expected_result);
		return 1;
	}
    if (rem != expected_rem) {
        printf("error in __divmodsi4: %d mod %d = %d, expected %d\n",
               a, b, rem, expected_rem);
		return 1;
	}
	
    return 0;
}


int main()
{
    if (test__divmodsi4(0, 1, 0, 0))
        return 1;
    if (test__divmodsi4(0, -1, 0, 0))
        return 1;

    if (test__divmodsi4(2, 1, 2, 0))
        return 1;
    if (test__divmodsi4(2, -1, -2, 0))
        return 1;
    if (test__divmodsi4(-2, 1, -2, 0))
        return 1;
    if (test__divmodsi4(-2, -1, 2, 0))
        return 1;

	if (test__divmodsi4(7, 5, 1, 2))
        return 1;
	if (test__divmodsi4(-7, 5, -1, -2))
        return 1;
	if (test__divmodsi4(19, 5, 3, 4))
        return 1;
	if (test__divmodsi4(19, -5, -3, 4))
        return 1;
  	
	if (test__divmodsi4(0x80000000, 8, 0xf0000000, 0))
        return 1;
	if (test__divmodsi4(0x80000007, 8, 0xf0000001, -1))
        return 1;

    return 0;
}
