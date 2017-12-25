/**
 * @file    vfs_manager.c
 * @brief   Implementation of vfs_manager.h
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
#include "task_main.h"
#include "virtual_fs.h"
#include "vfs_manager.h"
#include "file_stream.h"

#define INVALID_TIMEOUT_MS  0xFFFFFFFF
#define MAX_EVENT_TIME_MS   60000

#define CONNECT_DELAY_MS 0
#define RECONNECT_DELAY_MS 2500    // Must be above 1s for windows (more for linux)
// TRANSFER_IN_PROGRESS
#define DISCONNECT_DELAY_TRANSFER_TIMEOUT_MS 500 // was 20000 - this is triggered after a partial file upload
// TRANSFER_CAN_BE_FINISHED
#define DISCONNECT_DELAY_TRANSFER_IDLE_MS 500
// TRANSFER_NOT_STARTED || TRASNFER_FINISHED
#define DISCONNECT_DELAY_MS 500

bool vfs_is_windows = false;

// Make sure none of the delays exceed the max time
COMPILER_ASSERT(CONNECT_DELAY_MS < MAX_EVENT_TIME_MS);
COMPILER_ASSERT(RECONNECT_DELAY_MS < MAX_EVENT_TIME_MS);
COMPILER_ASSERT(DISCONNECT_DELAY_TRANSFER_TIMEOUT_MS < MAX_EVENT_TIME_MS);
COMPILER_ASSERT(DISCONNECT_DELAY_TRANSFER_IDLE_MS < MAX_EVENT_TIME_MS);
COMPILER_ASSERT(DISCONNECT_DELAY_MS < MAX_EVENT_TIME_MS);

volatile vfs_info_t vfs_info;

typedef enum {
    TRANSFER_NOT_STARTED,
    TRANSFER_IN_PROGRESS,
    TRANSFER_CAN_BE_FINISHED,
    TRASNFER_FINISHED,
} transfer_state_t;

typedef struct {
    vfs_file_t file_to_program;     // A pointer to the directory entry of the file being programmed
    vfs_sector_t start_sector;      // Start sector of the file being programmed
    vfs_sector_t file_next_sector;  // Expected next sector of the file
    vfs_sector_t last_ooo_sector;   // Last out of order sector within the file
    uint32_t size_processed;        // The number of bytes processed by the stream
    uint32_t file_size;             // Size of the file indicated by root dir.  Only allowed to increase
    uint32_t size_transferred;      // The number of bytes transferred
    transfer_state_t transfer_state;// Transfer state
    bool stream_open;               // State of the stream
    bool stream_started;            // Stream processing started. This only gets reset remount
    bool stream_finished;           // Stream processing is done. This only gets reset remount
    bool stream_optional_finish;    // True if the stream processing can be considered done
    bool file_info_optional_finish; // True if the file transfer can be considered done
    bool transfer_timeout;          // Set if the transfer was finished because of a timeout. This only gets reset remount
    stream_type_t stream;           // Current stream or STREAM_TYPE_NONE is stream is closed.  This only gets reset remount
} file_transfer_state_t;

typedef enum {
    VFS_MNGR_STATE_DISCONNECTED,
    VFS_MNGR_STATE_RECONNECTING,
    VFS_MNGR_STATE_CONNECTED
} vfs_mngr_state_t;

static const file_transfer_state_t default_transfer_state = {
    VFS_FILE_INVALID,
    VFS_INVALID_SECTOR,
    VFS_INVALID_SECTOR,
    VFS_INVALID_SECTOR,
    0,
    0,
    0,
    TRANSFER_NOT_STARTED,
    false,
    false,
    false,
    false,
    false,
    false,
    STREAM_TYPE_NONE,
};

//static uint32_t usb_buffer[VFS_SECTOR_SIZE / sizeof(uint32_t)];
static error_t fail_reason = E_SUCCESS;
static file_transfer_state_t file_transfer_state;

// These variables can be access from multiple threads
// so access to them must be synchronized
static vfs_mngr_state_t vfs_state;
static vfs_mngr_state_t vfs_state_next;
static bool vfs_next_remount_full = false;
static uint32_t time_usb_idle;

static osStaticMutexDef_t vfsMutexControlBlock;
static osSemaphoreId vfsMutexHandle = NULL;

// Synchronization functions
static void sync_init(void);
static void sync_assert_usb_thread(void);
static void sync_lock(void);
static void sync_unlock(void);

static bool changing_state(void);
static void build_filesystem(void);
static void file_change_handler(const vfs_filename_t filename, vfs_file_change_t change, vfs_file_t file, vfs_file_t new_file_data);
static void file_data_handler(uint32_t sector, const uint8_t *buf, uint32_t num_of_sectors);
static bool ready_for_state_change(void);
static void abort_remount(void);

static void transfer_update_file_info(vfs_file_t file, uint32_t start_sector, uint32_t size, stream_type_t stream);
static void transfer_reset_file_info(void);
static void transfer_stream_open(stream_type_t stream, uint32_t start_sector);
static void transfer_stream_data(uint32_t sector, const uint8_t *data, uint32_t size);
static void transfer_update_state(error_t status);


void vfs_mngr_fs_enable(bool enable)
{
    sync_lock();
    vfs_printf("Enable = %d", enable);

    if (enable) {
        if (VFS_MNGR_STATE_DISCONNECTED == vfs_state_next) {
            vfs_printf(" Switch to Connected");
            vfs_state_next = VFS_MNGR_STATE_CONNECTED;
        } else {
            vfs_printf(" no switch");
        };
    } else {
        vfs_printf(" Switch to DISconnected");
        vfs_state_next = VFS_MNGR_STATE_DISCONNECTED;
    }
    sync_unlock();
}

void vfs_mngr_fs_remount(bool force_full)
{
    sync_lock();

    // Only start a remount if in the connected state and not in a transition
    if (!changing_state() && (VFS_MNGR_STATE_CONNECTED == vfs_state)) {
        vfs_state_next = VFS_MNGR_STATE_RECONNECTING;
    }

    vfs_next_remount_full |= force_full;

    sync_unlock();
}

void vfs_mngr_init(bool enable)
{
    vfs_printf("vfs_mngr_init");
    sync_assert_usb_thread();
    build_filesystem();

    if (enable) {
        vfs_state = VFS_MNGR_STATE_CONNECTED;
        vfs_state_next = VFS_MNGR_STATE_CONNECTED;
        vfs_info.MediaReady = 1;
    } else {
        vfs_state = VFS_MNGR_STATE_DISCONNECTED;
        vfs_state_next = VFS_MNGR_STATE_DISCONNECTED;
        vfs_info.MediaReady = 0;
    }
    vfs_info.MediaChanged = 0;
}

void vfs_mngr_periodic(uint32_t elapsed_ms)
{
    bool change_state;
    vfs_mngr_state_t vfs_state_local;
    vfs_mngr_state_t vfs_state_local_prev;
    sync_assert_usb_thread();
    sync_lock();

    // Return immediately if the desired state has been reached
    if (!changing_state()) {
        sync_unlock();
        return;
    }

    change_state = ready_for_state_change();

    if (time_usb_idle < MAX_EVENT_TIME_MS) {
        time_usb_idle += elapsed_ms;
    }

    if (!change_state) {
        sync_unlock();
        return;
    }

    vfs_printf("vfs_mngr_periodic()\r\n");
    vfs_printf("   time_usb_idle=%i\r\n", time_usb_idle);
    vfs_printf("   transfer_state=%i\r\n", file_transfer_state.transfer_state);
    // Transition to new state
    vfs_state_local_prev = vfs_state;
    vfs_state = vfs_state_next;

    switch (vfs_state) {
        case VFS_MNGR_STATE_RECONNECTING:
            // Transition back to the connected state
            vfs_state_next = VFS_MNGR_STATE_CONNECTED;
            break;

        default:
            // No state change logic required in other states
            break;
    }

    vfs_state_local = vfs_state;
    time_usb_idle = 0;
    sync_unlock();
    // Processing when leaving a state
    vfs_printf("    state %i->%i\r\n", vfs_state_local_prev, vfs_state_local);

    bool want_notify_only = false; // Use this if the transfer timed out and we dont need full reconnect
    switch (vfs_state_local_prev) {
        case VFS_MNGR_STATE_DISCONNECTED:
            // No action needed
            break;

        case VFS_MNGR_STATE_RECONNECTING:
            // No action needed
            break;

        case VFS_MNGR_STATE_CONNECTED:
            // Close ongoing transfer if there is one
            if (file_transfer_state.transfer_state != TRASNFER_FINISHED) {
                vfs_printf("    transfer timeout\r\n");
                file_transfer_state.transfer_timeout = true;
                transfer_update_state(E_SUCCESS);
                want_notify_only = true;
            }

            assert_param(TRASNFER_FINISHED == file_transfer_state.transfer_state);
            vfs_user_disconnecting();
            break;
    }

    // Maybe make this configurable?
    //want_notify_only = false;

    // Processing when entering a state
    switch (vfs_state_local) {
        case VFS_MNGR_STATE_DISCONNECTED:
            vfs_printf("+DISCON");
            if (want_notify_only && !vfs_next_remount_full) {
                vfs_info.MediaChanged = 1;
                vfs_printf("Notify media change");
            } else {
                vfs_info.MediaReady = 0;
                vfs_printf("Reconnect mass storage");
            }
            vfs_next_remount_full = false;
            break;

        case VFS_MNGR_STATE_RECONNECTING:
            vfs_printf("+RECON");
            if (want_notify_only && !vfs_next_remount_full) {
                vfs_info.MediaChanged = 1;
                vfs_printf("Notify media change");
            } else {
                vfs_info.MediaReady = 0;
                vfs_printf("Reconnect mass storage");
            }
            vfs_next_remount_full = false;
            break;

        case VFS_MNGR_STATE_CONNECTED:
            vfs_printf("+CONNECTED");
            build_filesystem();
            vfs_info.MediaReady = 1;
            break;
    }

    return;
}

error_t vfs_mngr_get_transfer_status(void)
{
    sync_assert_usb_thread();
    return fail_reason;
}

void vfs_if_usbd_msc_init(void)
{
    sync_init();
    build_filesystem();
    vfs_state = VFS_MNGR_STATE_DISCONNECTED;
    vfs_state_next = VFS_MNGR_STATE_DISCONNECTED;
    time_usb_idle = 0;
    vfs_info.MediaReady = 0;
    vfs_info.MediaChanged = 0;
    vfs_printf("vfs_if_usbd_msc_init");
}

void vfs_if_usbd_msc_read_sect(uint32_t sector, uint8_t *buf, uint32_t num_of_sectors)
{
    sync_assert_usb_thread();
    // /* this is very spammy */ vfs_printf("\033[35mREAD @ %d, len %d\033[0m", sector, num_of_sectors);

    // dont proceed if we're not ready
    if (!vfs_info.MediaReady) {
        vfs_printf("Not Rdy");
        return;
    }

    // indicate msc activity
//    main_blink_msc_led(MAIN_LED_OFF);
    vfs_read(sector, buf, num_of_sectors);
}

void vfs_if_usbd_msc_write_sect(uint32_t sector, uint8_t *buf, uint32_t num_of_sectors)
{
    sync_assert_usb_thread();
    vfs_printf("\033[32mWRITE @ %d, len %d\033[0m", sector, num_of_sectors);
    if (buf[0] == 0xF8 && buf[1] == 0xFF && buf[2] == 0xFF && buf[3] == 0xFF) {
        vfs_printf("Discard write of F8,FF,FF,FF");
        return;
    }

    if (!vfs_info.MediaReady) {
        vfs_printf("Not Rdy");
        return;
    }

    // Restart the disconnect counter on every packet
    // so the device does not detach in the middle of a
    // transfer.
    time_usb_idle = 0;

    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        vfs_printf("Xfer done.");
        return;
    }

    // indicate msc activity
//    main_blink_msc_led(MAIN_LED_OFF);
    vfs_printf("call vfs_write");
    vfs_write(sector, buf, num_of_sectors);
    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        vfs_printf("Xfer done now.");
        return;
    }
    file_data_handler(sector, buf, num_of_sectors);
}

static void sync_init(void)
{
    osMutexStaticDef(vfsMutex, &vfsMutexControlBlock);
    vfsMutexHandle = osMutexCreate(osMutex(vfsMutex));
}

static inline void sync_assert_usb_thread(void)
{
    assert_param(osThreadGetId() == tskMainHandle);
}

static void sync_lock(void)
{
    assert_param(osOK == osMutexWait(vfsMutexHandle, 100));
}

static void sync_unlock(void)
{
    assert_param(osOK == osMutexRelease(vfsMutexHandle));
}

static bool changing_state(void)
{
    return vfs_state != vfs_state_next;
}

static void build_filesystem(void)
{
    // Update anything that could have changed file system state
    file_transfer_state = default_transfer_state;
    vfs_user_build_filesystem();
    vfs_set_file_change_callback(file_change_handler);
    // Set mass storage parameters

    vfs_info.MemorySize = vfs_get_total_size();
    vfs_info.BlockSize = VFS_SECTOR_SIZE;
    vfs_info.BlockGroup = 1;
    vfs_info.BlockCount = vfs_info.MemorySize / vfs_info.BlockSize;
//    vfs_info.BlockBuf = (uint8_t *) usb_buffer;
}

static void switch_to_new_file(stream_type_t stream, uint32_t start_sector, bool andReopen)
{ // This should close the stream

    vfs_printf("****** NEW FILE STREAM! *******");
    if (!file_transfer_state.transfer_state) {
        file_transfer_state.transfer_timeout = true;
        transfer_update_state(E_SUCCESS);
    } else {
        if (file_transfer_state.stream_open) {
            stream_close();
            file_transfer_state.stream_open = false;
        }
    }

    if (andReopen) {
        // and we start anew
        file_transfer_state.start_sector = VFS_INVALID_SECTOR; // pretend we have no srtart sector yet!
        transfer_stream_open(stream, start_sector);
    }
}

// Callback to handle changes to the root directory.  Should be used with vfs_set_file_change_callback
static void file_change_handler(const vfs_filename_t filename, vfs_file_change_t change, vfs_file_t file, vfs_file_t new_file_data)
{
    vfs_printf("\033[33m@file_change_handler\033[0m (name=%*s, file=%p, ftp=%p, change=%i)\r\n", 11, filename, file, change);

    vfs_user_file_change_handler(filename, change, file, new_file_data);
    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        // If the transfer is finished stop further processing
        vfs_printf("> Transfer is finished.");
        return;
    }

    if (VFS_FILE_CHANGED == change) {
        vfs_printf("> Change");
        if (file == file_transfer_state.file_to_program) {
            vfs_printf("  Stream is open, continue");
            stream_type_t stream;
            uint32_t size = vfs_file_get_size(new_file_data);
            vfs_sector_t sector = vfs_file_get_start_sector(new_file_data);
            stream = stream_type_from_name(filename);
            transfer_update_file_info(file, sector, size, stream);
        } else {
            vfs_printf("  No stream.");
        }
    }

    if (VFS_FILE_CREATED == change) {
        stream_type_t stream;
        vfs_printf("> Created");

        if (STREAM_TYPE_NONE != stream_type_from_name(filename)) {
            vfs_printf("  Stream is open, continue");

            // Check for a know file extension to detect the current file being
            // transferred.  Ignore hidden files since MAC uses hidden files with
            // the same extension to keep track of transfer info in some cases.
            if (!(VFS_FILE_ATTR_HIDDEN & vfs_file_get_attr(new_file_data))) {
                stream = stream_type_from_name(filename);
                uint32_t size = vfs_file_get_size(new_file_data);
                vfs_sector_t sector = vfs_file_get_start_sector(new_file_data);
                transfer_update_file_info(file, sector, size, stream);
            }
        } else {
            vfs_printf("  No matching stream found!");
        }
    }

    if (VFS_FILE_DELETED == change) {
        vfs_printf("> Deleted");
        if (file == file_transfer_state.file_to_program) {
            vfs_printf("  Deleted transferred file!");
            // The file that was being transferred has been deleted
            transfer_reset_file_info();
        } else {
            vfs_printf("Delete other file");
        }
    }
}

// Handler for file data arriving over USB.  This function is responsible
// for detecting the start of a BIN/HEX file and performing programming
static void file_data_handler(uint32_t sector, const uint8_t *buf, uint32_t num_of_sectors)
{
    stream_type_t stream;
    uint32_t size;
    vfs_printf("\033[33m@file_data_handler\033[0m (sec=%d, num=%d)", sector, num_of_sectors);

    if (sector <= 1) {
        vfs_printf("Discard write to sector %d", sector);
        return;
    }

    // this is the key for starting a file write - we dont care what file types are sent
    //  just look for something unique (NVIC table, hex, srec, etc) until root dir is updated
    if (!file_transfer_state.stream_started) {
        vfs_printf("Stream not started yet");
        // look for file types we can program
        stream = stream_start_identify((uint8_t *) buf, VFS_SECTOR_SIZE * num_of_sectors);

        if (STREAM_TYPE_NONE != stream) {
            vfs_printf("Opening a stream...");
            transfer_stream_open(stream, sector);
        }
    }

    if (file_transfer_state.stream_started) {
        vfs_printf("Stream is open, check if we can write ....");

//        // Ignore sectors coming before this file
//        if (sector < file_transfer_state.start_sector) {
//            vfs_printf("Sector ABOVE current file!?");
//            return;
//        }

        // sectors must be in order
        if (sector != file_transfer_state.file_next_sector) {
            vfs_printf("file_data_handler BAD sector=%i\r\n", sector);

            // Try to find what file this belongs to, if any
            // OS sometimes first writes the FAT and then the individual files,
            // so it looks to us as a discontinuous file (in the better case)
            vfs_filename_t fname;
            vfs_file_t *file;
            if (vfs_find_file(sector, &fname, &file)) {
                vfs_printf("FOUND A FILE!! matches to %s", fname);
                file_transfer_state.file_to_program = file;

                stream = stream_start_identify((uint8_t *) buf, VFS_SECTOR_SIZE * num_of_sectors);
                if (stream != STREAM_TYPE_NONE) {
                    switch_to_new_file(stream, sector, true);
                    goto proceed;
                }

                vfs_printf("No stream can handle this, give up. Could be kateswap junk (1)\r\n");
                return;
            }

            if (sector >= file_transfer_state.start_sector && sector < file_transfer_state.file_next_sector) {
                vfs_printf("    sector out of order! lowest ooo = %i\r\n",
                           file_transfer_state.last_ooo_sector);

                if (VFS_INVALID_SECTOR == file_transfer_state.last_ooo_sector) {
                    file_transfer_state.last_ooo_sector = sector;
                }

                file_transfer_state.last_ooo_sector =
                    MIN(file_transfer_state.last_ooo_sector, sector);
            } else {
                vfs_printf("    sector not part of file transfer\r\n");

                // BUT!! this can be a whole different file written elsewhere
                // Let's try it.
                if (sector > 70) {
                    // this is a guess as to where the actual data can start - usually 34 and 67 are some garbage in the FAT(s)

                    stream = stream_start_identify((uint8_t *) buf, VFS_SECTOR_SIZE * num_of_sectors);
                    if (stream != STREAM_TYPE_NONE) {
                        switch_to_new_file(stream, sector, true);
                        goto proceed;
                    }

                    vfs_printf("No stream can handle this, give up. Could be kateswap junk (2)\r\n");
                    return;
                }
            }

            vfs_printf("    discarding data - size transferred=0x%x\r\n",
                       file_transfer_state.size_transferred);

            vfs_printf_nonl("\033[31m", 5);
            // FIXME this seems wrong
            vfs_printf_nonl((const char *) buf, VFS_SECTOR_SIZE * num_of_sectors);
            vfs_printf_nonl("\033[0m\r\n", 6);

            return;
        } else {
            vfs_printf("sector is good");
        }
        proceed:
        // This sector could be part of the file so record it
        size = VFS_SECTOR_SIZE * num_of_sectors;
        file_transfer_state.size_transferred += size;
        file_transfer_state.file_next_sector = sector + num_of_sectors;

        // If stream processing is done then discard the data
        if (file_transfer_state.stream_finished) {
            vfs_printf("vfs_manager file_data_handler\r\n    sector=%i, size=%i\r\n", sector, size);
            vfs_printf("    discarding data - size transferred=0x%x\r\n",
                       file_transfer_state.size_transferred);

            vfs_printf_nonl("\033[31m", 5);
            vfs_printf_nonl((const char *) buf, VFS_SECTOR_SIZE * num_of_sectors);
            vfs_printf_nonl("\033[0m\r\n", 6);

            transfer_update_state(E_SUCCESS);
            return;
        } else {
            vfs_printf("stream is not finished, can handle...");
        }

        transfer_stream_data(sector, buf, size);
    } else {
        vfs_printf("Stream not started!!!!!!");
    }
}

static bool ready_for_state_change(void)
{
    uint32_t timeout_ms = INVALID_TIMEOUT_MS;
    assert_param(vfs_state != vfs_state_next);

    if (VFS_MNGR_STATE_CONNECTED == vfs_state) {
        switch (file_transfer_state.transfer_state) {
            case TRANSFER_NOT_STARTED:
            case TRASNFER_FINISHED:
                timeout_ms = DISCONNECT_DELAY_MS;
                break;

            case TRANSFER_IN_PROGRESS:
                timeout_ms = DISCONNECT_DELAY_TRANSFER_TIMEOUT_MS;
                break;

            case TRANSFER_CAN_BE_FINISHED:
                timeout_ms = DISCONNECT_DELAY_TRANSFER_IDLE_MS;
                break;

            default:
                trap("Bad xfer state");
                timeout_ms = DISCONNECT_DELAY_MS;
                break;
        }
    } else if ((VFS_MNGR_STATE_DISCONNECTED == vfs_state) &&
               (VFS_MNGR_STATE_CONNECTED == vfs_state_next)) {
        timeout_ms = CONNECT_DELAY_MS;
    } else if ((VFS_MNGR_STATE_RECONNECTING == vfs_state) &&
               (VFS_MNGR_STATE_CONNECTED == vfs_state_next)) {
        timeout_ms = RECONNECT_DELAY_MS;
    } else if ((VFS_MNGR_STATE_RECONNECTING == vfs_state) &&
               (VFS_MNGR_STATE_DISCONNECTED == vfs_state_next)) {
        timeout_ms = 0;
    }

    if (INVALID_TIMEOUT_MS == timeout_ms) {
        trap("invalid timeout");
        timeout_ms = 0;
    }

    return time_usb_idle > timeout_ms ? true : false;
}

// Abort a remount if one is pending
void abort_remount(void)
{
    sync_lock();

    // Only abort a remount if in the connected state and reconnecting is the next state
    if ((VFS_MNGR_STATE_RECONNECTING == vfs_state_next) && (VFS_MNGR_STATE_CONNECTED == vfs_state)) {
        vfs_state_next = VFS_MNGR_STATE_CONNECTED;
    }

    sync_unlock();
}

// Update the tranfer state with file information
static void transfer_update_file_info(vfs_file_t file, uint32_t start_sector, uint32_t size, stream_type_t stream)
{
    vfs_printf("\033[33m@transfer_update_file_info\033[0m (file=%p, start_sector=%i, size=%i)\r\n", file, start_sector, size);

    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        trap("xfer already finished");
        return;
    }

    // Initialize the directory entry if it has not been set
    if (VFS_FILE_INVALID == file_transfer_state.file_to_program) {
        file_transfer_state.file_to_program = file;

        if (file != VFS_FILE_INVALID) {
            vfs_printf("    file_to_program=%p\r\n", file);
        }
    }

    // Initialize the starting sector if it has not been set
    if (VFS_INVALID_SECTOR == file_transfer_state.start_sector) {
        file_transfer_state.start_sector = start_sector;

        if (start_sector != VFS_INVALID_SECTOR) {
            vfs_printf("    start_sector=%i\r\n", start_sector);
        }
    }

    // Initialize the stream if it has not been set
    if (STREAM_TYPE_NONE == file_transfer_state.stream) {
        file_transfer_state.stream = stream;

        if (stream != STREAM_TYPE_NONE) {
            vfs_printf("    stream=%i\r\n", stream);
        }
    }

    // Check - File size must either grow or be smaller than the size already transferred
    if ((size < file_transfer_state.file_size) && (size < file_transfer_state.size_transferred)) {
        vfs_printf("    error: file size changed from %i to %i\r\n", file_transfer_state.file_size, size);
        // this is probably a new file
        trap("File shrinks");//XXX
        switch_to_new_file(stream, start_sector, true);
    }

    // Check - Starting sector must be the same  - this is optional for file info since it may not be present initially
    if ((VFS_INVALID_SECTOR != start_sector) && (start_sector != file_transfer_state.start_sector)) {
        vfs_printf("    error: starting sector changed from %i to %i\r\n", file_transfer_state.start_sector, start_sector);
        // this is probably a new file

        trap("Changed start offset");//FIXME this sometimes happens, need to find how to reproduce
        switch_to_new_file(stream, start_sector, true);
    }

    // Check - stream must be the same
    if (stream != file_transfer_state.stream) {
        vfs_printf("    error: changed types during transfer from %i to %i\r\n", stream, file_transfer_state.stream);
        transfer_update_state(E_ERROR_DURING_TRANSFER);
        return;
    }

    // Update values - Size is the only value that can change
    file_transfer_state.file_size = size;
    vfs_printf("    updated size=%i\r\n", size);

    transfer_update_state(E_SUCCESS);
}

// Reset the transfer information or error if transfer is already in progress
static void transfer_reset_file_info(void)
{
    vfs_printf("vfs_manager transfer_reset_file_info()\r\n");
    if (file_transfer_state.stream_open) {
        transfer_update_state(E_ERROR_DURING_TRANSFER);
    } else {
        file_transfer_state = default_transfer_state;
        abort_remount();
    }
}

// Update the tranfer state with new information
static void transfer_stream_open(stream_type_t stream, uint32_t start_sector)
{
    error_t status;
    assert_param(!file_transfer_state.stream_open);
    assert_param(start_sector != VFS_INVALID_SECTOR);
    vfs_printf("\033[33m@transfer_stream_open\033[0m (stream=%i, start_sector=%i)\r\n",
               stream, start_sector);

    // Check - Starting sector must be the same
    if (start_sector != file_transfer_state.start_sector && file_transfer_state.start_sector != VFS_INVALID_SECTOR) {
        vfs_printf("    error: starting sector changed from %i to %i\r\n", file_transfer_state.start_sector, start_sector);
        // this is probably a new file
        switch_to_new_file(stream, start_sector, false);
        file_transfer_state.start_sector = VFS_INVALID_SECTOR;
    }

    // Check - stream must be the same
    if (stream != file_transfer_state.stream && file_transfer_state.stream != STREAM_TYPE_NONE) {
        vfs_printf("    error: changed types during tranfer from %i to %i\r\n", stream, file_transfer_state.stream);
        // this is probably a new file
        switch_to_new_file(stream, start_sector, false);
        file_transfer_state.start_sector = VFS_INVALID_SECTOR;
    }

    // Initialize the starting sector if it has not been set
    if (VFS_INVALID_SECTOR == file_transfer_state.start_sector) {
        file_transfer_state.start_sector = start_sector;

        if (start_sector != VFS_INVALID_SECTOR) {
            vfs_printf("    start_sector=%i\r\n", start_sector);
        }
    }

    // Initialize the stream if it has not been set
    if (STREAM_TYPE_NONE == file_transfer_state.stream) {
        file_transfer_state.stream = stream;

        if (stream != STREAM_TYPE_NONE) {
            vfs_printf("    stream=%i\r\n", stream);
        }
    }

    // Open stream
    status = stream_open(stream);
    vfs_printf("    stream_open stream=%i ret %i\r\n", stream, status);

    if (E_SUCCESS == status) {
        file_transfer_state.file_next_sector = start_sector;
        file_transfer_state.stream_open = true;
        file_transfer_state.stream_started = true;
    }

    transfer_update_state(status);
}

// Update the tranfer state with new information
static void transfer_stream_data(uint32_t sector, const uint8_t *data, uint32_t size)
{
    error_t status;
    vfs_printf("\033[33m@transfer_stream_data\033[0m (sector=%i, size=%i)\r\n", sector, size);
    vfs_printf("    size processed=0x%x, data=%x,%x,%x,%x,...\r\n",
               file_transfer_state.size_processed, data[0], data[1], data[2], data[3]);

    if (file_transfer_state.stream_finished) {
        trap("Stream already closed");
        return;
    }

    assert_param(size % VFS_SECTOR_SIZE == 0);
    assert_param(file_transfer_state.stream_open);
    status = stream_write((uint8_t *) data, size);
    vfs_printf("    stream_write ret=%i\r\n", status);

    if (E_SUCCESS_DONE == status) {
        // Override status so E_SUCCESS_DONE
        // does not get passed into transfer_update_state
        status = stream_close();
        vfs_printf("    stream_close ret=%i\r\n", status);
        file_transfer_state.stream_open = false;
        file_transfer_state.stream_finished = true;
        file_transfer_state.stream_optional_finish = true;
    } else if (E_SUCCESS_DONE_OR_CONTINUE == status) {
        status = E_SUCCESS;
        file_transfer_state.stream_optional_finish = true;
    } else {
        file_transfer_state.stream_optional_finish = false;
    }

    file_transfer_state.size_processed += size;
    transfer_update_state(status);
}

// Check if the current transfer is still in progress, done, or if an error has occurred
static void transfer_update_state(error_t status)
{
    bool transfer_timeout;
    bool transfer_started;
    bool transfer_can_be_finished;
    bool transfer_must_be_finished;
    bool out_of_order_sector;
    error_t local_status = status;
    assert_param((status != E_SUCCESS_DONE) &&
                 (status != E_SUCCESS_DONE_OR_CONTINUE));

    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        trap("Xfer already closed");
        return;
    }

    // Update file info status.  The end of a file is never known for sure since
    // what looks like a complete file could be part of a file getting flushed to disk.
    // The criteria for an successful optional finish is
    // 1. A file has been detected
    // 2. The size of the file indicated in the root dir has been transferred
    // 3. The file size is greater than zero
    file_transfer_state.file_info_optional_finish =
        (file_transfer_state.file_to_program != VFS_FILE_INVALID) &&
        (file_transfer_state.size_transferred >= file_transfer_state.file_size) &&
        (file_transfer_state.file_size > 0);
    transfer_timeout = file_transfer_state.transfer_timeout;
    transfer_started = (VFS_FILE_INVALID != file_transfer_state.file_to_program) ||
                       (STREAM_TYPE_NONE != file_transfer_state.stream);
    // The transfer can be finished if both file and stream processing
    // can be considered complete
    transfer_can_be_finished = file_transfer_state.file_info_optional_finish &&
                               file_transfer_state.stream_optional_finish;
    // The transfer must be fnished if stream processing is for sure complete
    // and file processing can be considered complete
    transfer_must_be_finished = file_transfer_state.stream_finished &&
                                file_transfer_state.file_info_optional_finish;
    out_of_order_sector = false;

    if (file_transfer_state.last_ooo_sector != VFS_INVALID_SECTOR) {
        assert_param(file_transfer_state.start_sector != VFS_INVALID_SECTOR);
        uint32_t sector_offset = (file_transfer_state.last_ooo_sector -
                                  file_transfer_state.start_sector) * VFS_SECTOR_SIZE;

        if (sector_offset < file_transfer_state.size_processed) {
            // The out of order sector was within the range of data already
            // processed.
            out_of_order_sector = true;
        }
    }

    // Set the transfer state and set the status if necessary
    if (local_status != E_SUCCESS) {
        file_transfer_state.transfer_state = TRASNFER_FINISHED;
    } else if (transfer_timeout) {
        if (out_of_order_sector) {
            local_status = E_OOO_SECTOR;
        } else if (!transfer_started) {
            local_status = E_SUCCESS;
        } else if (transfer_can_be_finished) {
            local_status = E_SUCCESS;
        } else {
            local_status = E_TRANSFER_TIMEOUT;
        }

        file_transfer_state.transfer_state = TRASNFER_FINISHED;
    } else if (transfer_must_be_finished) {
        file_transfer_state.transfer_state = TRASNFER_FINISHED;
    } else if (transfer_can_be_finished) {
        file_transfer_state.transfer_state = TRANSFER_CAN_BE_FINISHED;
    } else if (transfer_started) {
        file_transfer_state.transfer_state = TRANSFER_IN_PROGRESS;
    }

    if (TRASNFER_FINISHED == file_transfer_state.transfer_state) {
        vfs_printf("vfs_manager transfer_update_state(status=%i)\r\n", status);
        vfs_printf("    file=%p, start_sect= %i, size=%i\r\n",
                   file_transfer_state.file_to_program, file_transfer_state.start_sector,
                   file_transfer_state.file_size);
        vfs_printf("    stream=%i, size_processed=%i, opt_finish=%i, timeout=%i\r\n",
                   file_transfer_state.stream, file_transfer_state.size_processed,
                   file_transfer_state.file_info_optional_finish, transfer_timeout);

        // Close the file stream if it is open
        if (file_transfer_state.stream_open) {
            error_t close_status;
            close_status = stream_close();
            vfs_printf("    stream closed ret=%i\r\n", close_status);
            file_transfer_state.stream_open = false;

            if (E_SUCCESS == local_status) {
                local_status = close_status;
            }
        }

        // Set the fail reason
        fail_reason = local_status;
        vfs_printf("    Transfer finished, status: %i=%s\r\n", fail_reason, error_get_string(fail_reason));
    }

    // If this state change is not from aborting a transfer
    // due to a remount then trigger a remount
    if (!transfer_timeout) {
        vfs_printf("~~ request Remount from transfer_update_state()");
        vfs_mngr_fs_remount(false);
    }
}
