/**
 * @file    vfs_user.c
 * @brief   Implementation of vfs_user.h
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

#include "utils/ini_writer.h"
#include "framework/settings.h"
#include "platform.h"
#include "vfs_manager.h"

const vfs_filename_t daplink_drive_name = "VIRTUALFS";

// File callback to be used with vfs_add_file to return file contents
static uint32_t read_file_config_ini(uint32_t sector_offset, uint8_t *data, uint32_t num_sectors)
{
    vfs_printf("Read config.ini");

    const uint32_t avail = num_sectors*VFS_SECTOR_SIZE;
    const uint32_t skip = sector_offset*VFS_SECTOR_SIZE;

    IniWriter iw = iw_init((char *)data, skip, avail);
    settings_write_ini(&iw);

    return avail - iw.count;
}


//
static void write_file_config_ini(uint32_t sector_offset, const uint8_t *data, uint32_t num_sectors)
{
    vfs_printf("Write CONFIG.INI, so %d, ns %d", sector_offset, num_sectors);

    for(uint32_t i=0;i<num_sectors*VFS_SECTOR_SIZE;i++) {
        PRINTF("%c", data[i]);
    }
}


void vfs_user_build_filesystem(void)
{
    dbg("Rebuilding VFS...");

    // Setup the filesystem based on target parameters
    vfs_init(daplink_drive_name, 0/*unused*/);

    vfs_create_file("CONFIG  INI", read_file_config_ini, write_file_config_ini, settings_get_ini_len());
}

// Callback to handle changes to the root directory.  Should be used with vfs_set_file_change_callback
void vfs_user_file_change_handler(const vfs_filename_t filename,
                                  vfs_file_change_t change,
                                  vfs_file_t file, vfs_file_t new_file_data)
{
    if (VFS_FILE_CHANGED == change) {
        // Unused
        vfs_printf(">>> CHANGED %s", filename);
    }

    if (VFS_FILE_CREATED == change) {
        // do something based on the filename here
        vfs_printf(">>> CREATED %s", filename);
    }

    if (VFS_FILE_DELETED == change) {
        //
        vfs_printf(">>> DELETED %s", filename);
    }
}

void vfs_user_disconnecting(void)
{
    // maybe reset...
    vfs_printf("vfs_user_disconnecting");
}
