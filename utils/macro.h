/**
 * @file    macro.h
 * @brief   useful things + Special asserts and macros
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VFS_MACRO_H
#define VFS_MACRO_H

#ifdef __cplusplus
extern "C" {
#endif

// --------------- VFS macros and general purpose --------------------

#define ELEMENTS_IN_ARRAY(array)        (sizeof(array)/sizeof(array[0]))

#define MB(size)                        ((size) * 1024 * 1024)

#define KB(size)                        ((size) * 1024)

#ifndef MIN
#define MIN(a,b)                        (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)                        (((a) > (b)) ? (a) : (b))
#endif

#define ROUND_UP(value, boundary)       ((value) + ((boundary) - (value)) % (boundary))

#define ROUND_DOWN(value, boundary)     ((value) - ((value) % (boundary)))

// ---------- HELPERS FOR XMACROS -----------------

#define XJOIN(a, b) a##b
#define STR_(x) #x
#define STR(x) STR_(x)

// ---------- COMPILER SPECIAL MACROS -------------

#define COMPILER_CONCAT_(a, b) a##b
#define COMPILER_CONCAT(a, b) COMPILER_CONCAT_(a, b)

// Divide by zero if the the expression is false.  This
// causes an error at compile time.
//
// The special value '__COUNTER__' is used to create a unique value to
// append to 'compiler_assert_' to create a unique token.  This prevents
// conflicts resulting from the same enum being declared multiple times.
#define COMPILER_ASSERT(e) enum { COMPILER_CONCAT(compiler_assert_, __COUNTER__) = 1/((e) ? 1 : 0) }

#define __at(_addr) __attribute__ ((at(_addr)))

#define VA_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#ifdef __cplusplus
}
#endif

#endif
