/**
 * @file    error.c
 * @brief   Implementation of error.h
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

#include "platform.h"

static const char *const error_names[] = {
#define X(name, text) STR(name),
    X_ERROR_CODES
#undef X
};
COMPILER_ASSERT(ERROR_COUNT == ELEMENTS_IN_ARRAY(error_names));

static const char *const error_message[] = {
#define X(name, text) text,
    X_ERROR_CODES
#undef X
};
COMPILER_ASSERT(ERROR_COUNT == ELEMENTS_IN_ARRAY(error_message));

const char *error_get_message(error_t error)
{
    const char *msg = NULL;

    if (error < ERROR_COUNT) {
        msg = error_message[error];
    }

    if (NULL == msg) {
        return error_get_name(error);
    }

    return msg;
}

const char *error_get_name(error_t error)
{
    const char *name = 0;

    if (error < ERROR_COUNT) {
        name = error_names[error];
    }

    if (0 == name) {
        dbg("%d", (int)error);
        trap("MISSING ERROR NAME");
    }

    return name;
}
