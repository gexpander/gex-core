/**
 * @file    error.h
 * @brief   collection of known errors and accessor for the friendly string
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

#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
}
#endif

#define X_ERROR_CODES \
    /* Shared errors */  \
    X(SUCCESS,"OK") \
    X(FAILURE,"Failure") \
    X(INTERNAL,"Internal error") \
    X(LOADING,"Init in progress") \
    \
    /* VFS user errors */ \
    X(ERROR_DURING_TRANSFER,"Error during transfer") \
    X(TRANSFER_TIMEOUT,"Transfer timed out") \
    /*X(FILE_BOUNDS,"Possible mismatch between file size and size programmed")*/ \
    X(OOO_SECTOR,"File sent out of order by PC") \
    \
    /* File stream errors */ \
    X(SUCCESS_DONE,"End of stream") \
    X(SUCCESS_DONE_OR_CONTINUE,"End of stream, or more to come") \
    \
    /* Unit loading */ \
    X(BAD_CONFIG, "Configuration error") \
    X(OUT_OF_MEM, "Not enough RAM") \
    X(RESOURCE_NOT_AVAILABLE, "Required pin / peripheral not available")

// Keep in sync with the list error_message
typedef enum {
#define X(name, text) E_##name,
    X_ERROR_CODES
#undef X
    ERROR_COUNT
} error_t;

const char *error_get_string(error_t error) __attribute__((pure));

#ifdef __cplusplus
}
#endif

#endif
