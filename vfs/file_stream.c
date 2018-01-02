/**
 * @file    file_stream.c
 * @brief   Implementation of file_stream.h
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

#include <platform/status_led.h>
#include "platform.h"
#include "utils/ini_parser.h"
#include "framework/settings.h"
#include "file_stream.h"
#include "vfs_manager.h"

typedef enum {
    STREAM_STATE_CLOSED,
    STREAM_STATE_OPEN,
    STREAM_STATE_END,
    STREAM_STATE_ERROR
} stream_state_t;

typedef bool (*stream_detect_cb_t)(const uint8_t *data, uint32_t size);
typedef error_t (*stream_open_cb_t)(void *state);
typedef error_t (*stream_write_cb_t)(void *state, const uint8_t *data, uint32_t size);
typedef error_t (*stream_close_cb_t)(void *state);

typedef struct {
    stream_detect_cb_t detect;
    stream_open_cb_t open;
    stream_write_cb_t write;
    stream_close_cb_t close;
} stream_t;

typedef struct {
    size_t file_pos;
} conf_state_t;

typedef union {
    conf_state_t conf;
} shared_state_t;

static bool detect_conf(const uint8_t *data, uint32_t size);
static error_t open_conf(void *state);
static error_t write_conf(void *state, const uint8_t *data, uint32_t size);
static error_t close_conf(void *state);

stream_t stream[] = {
    {detect_conf, open_conf, write_conf, close_conf},   // STREAM_TYPE_CONF
};
COMPILER_ASSERT(ELEMENTS_IN_ARRAY(stream) == STREAM_TYPE_COUNT);
// STREAM_TYPE_NONE must not be included in count
COMPILER_ASSERT(STREAM_TYPE_NONE > STREAM_TYPE_COUNT);

static shared_state_t shared_state;
static stream_state_t stream_state = STREAM_STATE_CLOSED;
static stream_t *current_stream = 0;

stream_type_t stream_start_identify(const uint8_t *data, uint32_t size)
{
    stream_type_t i;

    for (i = STREAM_TYPE_START; i < STREAM_TYPE_COUNT; i++) {
        if (stream[i].detect(data, size)) {
            return i;
        }
    }

    return STREAM_TYPE_NONE;
}

// Identify the file type from its extension
stream_type_t stream_type_from_name(const vfs_filename_t filename)
{
    // 8.3 file names must be in upper case
    if (0 == strncmp("INI", &filename[8], 3)) {
        // This is used only to verify we identified the file correctly (?)
        return STREAM_TYPE_CONF;
    } else {
        return STREAM_TYPE_NONE;
    }
}

error_t stream_open(stream_type_t stream_type)
{
    error_t status;

    // Stream must not be open already
    if (stream_state != STREAM_STATE_CLOSED) {
        vfs_printf("!! Stream is not closed, cant open");
        assert_param(0);
        return E_INTERNAL;
    }

    // Stream must be of a supported type
    if (stream_type >= STREAM_TYPE_COUNT) {
        vfs_printf("!! Stream bad type");
        assert_param(0);
        return E_INTERNAL;
    }

    Indicator_Effect(STATUS_DISK_BUSY);
    // TODO create a thread...?

    // Initialize all variables
    memset(&shared_state, 0, sizeof(shared_state));
    stream_state = STREAM_STATE_OPEN;
    current_stream = &stream[stream_type];
    // Initialize the specified stream
    status = current_stream->open(&shared_state);

    if (E_SUCCESS != status) {
        stream_state = STREAM_STATE_ERROR;
        vfs_printf("!! not success");
    }

    return status;
}

error_t stream_write(const uint8_t *data, uint32_t size)
{
    error_t status;

    // Stream must be open already
    if (stream_state != STREAM_STATE_OPEN) {
        vfs_printf("!! Stream is not open, cant write");
        assert_param(0);
        return E_INTERNAL;
    }

    // Check thread after checking state since the stream thread is
    // set only if stream_open has been called
    //stream_thread_assert(); // ???

    // Write to stream
    status = current_stream->write(&shared_state, data, size);

    if (E_SUCCESS_DONE == status) {
        vfs_printf("Stream DONE");
        stream_state = STREAM_STATE_END;
    } else if ((E_SUCCESS_DONE_OR_CONTINUE == status) || (E_SUCCESS == status)) {
        // Stream should remain in the open state
        assert_param(STREAM_STATE_OPEN == stream_state);
        vfs_printf("Stream may close or get more data.,,");
    } else {
        stream_state = STREAM_STATE_ERROR;
        vfs_printf("!! FAIL in stream");
    }

    return status;
}

error_t stream_close(void)
{
    error_t status;

    // Stream must not be closed already
    if (STREAM_STATE_CLOSED == stream_state) {
        vfs_printf("!! Stream already closed");
        assert_param(0);
        return E_INTERNAL;
    }

    // Check thread after checking state since the stream thread is
    // set only if stream_open has been called
//    stream_thread_assert(); // ???
    // Close stream
    Indicator_Off(STATUS_DISK_BUSY);
    status = current_stream->close(&shared_state);
    stream_state = STREAM_STATE_CLOSED;
    return status;
}

static bool detect_conf(const uint8_t *data, uint32_t size)
{
    // Here we have received the first sector of a potential INI file (assuming it's a whole sector, since
    // this is called from the MSC driver).
    //
    // We can start parsing and look for the first section or some other marker. The file name is yet unknown
    // and may not be known for a while - we cannot use that to detect anything, unless we buffer the entire file
    // (a bad idea)

    // Our INI files always start with two hashes
    return size >= 2 && data[0] == '#' && data[1] == '#';
}

static void iniparser_cb(const char *section, const char *key, const char *value, void *userData)
{
    settings_load_ini_key(section, key, value);
}

static error_t open_conf(void *state)
{
    conf_state_t *conf = state;
    conf->file_pos = 0;
    vfs_printf("\r\n---- INI OPEN! ----");

    settings_load_ini_begin();
    ini_parse_begin(iniparser_cb, NULL);

    return E_SUCCESS;
}

static error_t write_conf(void *state, const uint8_t *data, uint32_t size)
{
    conf_state_t *conf = state;
    conf->file_pos += size;

    vfs_printf("Writing INI - RX %d bytes", size);
    vfs_printf_nonl("\033[92m", 5);
    vfs_printf_nonl((const char *) data, size);
    vfs_printf_nonl("\033[0m\r\n", 6);

    ini_parse((const char *) data, size);

    return E_SUCCESS_DONE_OR_CONTINUE; // indicate we don't really know if it's over or not
    // TODO use some marker for EOF in the actual config files
}

static error_t close_conf(void *state)
{
    conf_state_t *conf = state;
    vfs_printf("Close INI, total bytes = %d", conf->file_pos);

    ini_parse_end();
    settings_load_ini_end();

    // force a full remount to have the changes be visible
    vfs_mngr_fs_remount(true);

    return E_SUCCESS;
}
