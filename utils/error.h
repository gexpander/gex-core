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

// fields: name, msg. If msg is NULL, name as string will be used instead.
#define X_ERROR_CODES \
    /* Shared errors */  \
    X(SUCCESS, NULL) /* operation succeeded / unit loaded. Must be 0 */ \
    X(FAILURE, NULL) /* generic error */   \
    X(INTERNAL_ERROR, NULL) /* a bug */    \
    X(LOADING, NULL) /* unit is loading */ \
    X(UNKNOWN_COMMAND, NULL) \
    X(BAD_COUNT, NULL) \
    X(MALFORMED_COMMAND, NULL) \
    X(NOT_APPLICABLE, NULL) \
    X(HW_TIMEOUT, NULL) \
    X(NO_SUCH_UNIT, NULL) \
    X(OVERRUN, NULL) /* used in bulk transfer */ \
    X(PROTOCOL_BREACH, NULL) /* eating with the wrong spoon */  \
    \
    /* VFS user errors (those are meant to be shown to user) */ \
    X(ERROR_DURING_TRANSFER, "Error during transfer") \
    X(TRANSFER_TIMEOUT, "Transfer timed out") \
    X(OOO_SECTOR, "File sent out of order by PC") \
    \
    /* File stream errors/status */ \
    X(SUCCESS_DONE, NULL) \
    X(SUCCESS_DONE_OR_CONTINUE, NULL) \
    \
    /* Unit loading */ \
    X(BAD_CONFIG, "Configuration error") \
    X(BAD_KEY, "Unexpected config key") \
    X(BAD_VALUE, "Bad config value") \
    X(OUT_OF_MEM, "Not enough RAM") \
    X(RESOURCE_NOT_AVAILABLE, "Required pin / peripheral not available")

// Keep in sync with the list error_message
typedef enum {
#define X(name, text) E_##name,
    X_ERROR_CODES
#undef X
    ERROR_COUNT
} error_t;

const char *error_get_message(error_t error) __attribute__((pure));
const char *error_get_name(error_t error) __attribute__((pure));

/** Check return value and return it if not E_SUCCESS */
#define TRY(call) do { \
    error_t _rv; \
    _rv = call; \
    if (E_SUCCESS != _rv) return _rv; \
} while (0)

#ifdef __cplusplus
}
#endif

#endif
