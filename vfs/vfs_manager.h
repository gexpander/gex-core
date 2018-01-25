//
// The main VFS state machine, mostly based on DAPLink
//
// TODO many errors were originally written to a FAIL.TXT file for the user to see,
//      those are now caught by assert_param(0) and crash the whole system. This is not ideal.
//

/**
 * @file    vfs_manager.h
 * @brief   Methods that build and manipulate a virtual file system
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

#ifndef VFS_MANAGER_USER_H
#define VFS_MANAGER_USER_H

#include "platform.h"
#include "virtual_fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Flag that we're plugged into Windows.
 * This is detected by characteristic writes of some system metadata store (which we discard)
 */
extern bool vfs_is_windows;

/* Callable from anywhere */

/** Enable or disable the virtual filesystem */
void vfs_mngr_fs_enable(bool enabled);

/**
 * Remount the virtual filesystem
 *
 * @param force_full - use media ready toggle instead of just notifying of modified data
 *                     (this should be more reliable, but can also be more intrusive)
 */
void vfs_mngr_fs_remount(bool force_full);


/* Callable only from the thread running the virtual fs */

/**
 * Initialize the VFS manager
 * Must be called after USB has been initialized (usbd_init())
 *
 * @note Must only be called from the thread runnning USB
 * @param enabled
 */
void vfs_mngr_init(bool enabled);


/**
 * Run the vfs manager state machine
 *
 * @note Must only be called from the thread runnning USB
 * @param elapsed_ms
 */
void vfs_mngr_periodic(uint32_t elapsed_ms);

/**
 * Return the status of the last transfer or E_SUCCESS
 * if none have been performed yet
 *
 * @return success
 */
error_t vfs_mngr_get_transfer_status(void);


/* Use functions */

/**
 * Build the filesystem by calling vfs_init and then adding files with vfs_create_file
 */
void vfs_user_build_filesystem(void);

/**
 * Called when a file on the filesystem changes
 *
 * @param filename - name of the changed file
 * @param change - type of change
 * @param file - data pointer (?)
 * @param new_file_data - new data pointer (?)
 */
void vfs_user_file_change_handler(const vfs_filename_t filename,
                                  vfs_file_change_t change,
                                  vfs_file_t file, vfs_file_t new_file_data);

/**
 * Called when VFS is disconnecting
 */
void vfs_user_disconnecting(void);

// --- interface ---

/**
 * Initialize, call form the MSC init callback
 */
void vfs_if_usbd_msc_init(void);

/**
 * MSC wants to read a sector
 *
 * @param sector - first sector number
 * @param buf - destination
 * @param num_of_sectors - length
 */
void vfs_if_usbd_msc_read_sect(uint32_t sector, uint8_t *buf, uint32_t num_of_sectors);

/**
 * MSC wants to write a sector
 *
 * @param sector - first sector number
 * @param buf - data
 * @param num_of_sectors - length
 */
void vfs_if_usbd_msc_write_sect(uint32_t sector, const uint8_t *buf, uint32_t num_of_sectors);

typedef struct {
    uint32_t MemorySize;
    uint16_t BlockSize;
    uint32_t BlockGroup; // LUN
    uint32_t BlockCount;
    bool MediaReady;
    bool MediaChanged;
} vfs_info_t;

/** VFS info struct - some are used by SCSI/MSC */
extern volatile vfs_info_t vfs_info;

#ifdef __cplusplus
}
#endif

#endif// VFS_MANAGER_USER_H
