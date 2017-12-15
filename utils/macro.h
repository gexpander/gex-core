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
#define MIN(a,b)                        ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)                        ((a) > (b) ? (a) : (b))
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

#ifdef __cplusplus
}
#endif

#endif
