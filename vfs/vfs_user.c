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

#include "platform.h"
#include "utils/ini_writer.h"
#include "framework/settings.h"
#include "vfs_manager.h"
#include "str_utils.h"

const vfs_filename_t daplink_drive_name = VFS_DRIVE_NAME;

static uint32_t read_iw_sector(uint32_t sector_offset, uint8_t *data, uint32_t num_sectors, void (*handler)(IniWriter *))
{
    const uint32_t avail = num_sectors*VFS_SECTOR_SIZE;
    const uint32_t skip = sector_offset*VFS_SECTOR_SIZE;
    IniWriter iw = iw_init((char *)data, skip, avail);
    iw_begin();
    handler(&iw);
    iw_end();
    return avail - iw.count;
}

// File callback to be used with vfs_add_file to return file contents
static uint32_t read_file_units_ini(uint32_t sector_offset, uint8_t *data, uint32_t num_sectors)
{
    vfs_printf("Read UNITS.INI");
    return read_iw_sector(sector_offset, data, num_sectors, settings_build_units_ini);
}

static uint32_t read_file_system_ini(uint32_t sector_offset, uint8_t *data, uint32_t num_sectors)
{
    vfs_printf("Read SYSTEM.INI");
    return read_iw_sector(sector_offset, data, num_sectors, settings_build_system_ini);
}

static uint32_t read_file_pinout_txt(uint32_t sector_offset, uint8_t *data, uint32_t num_sectors)
{
    vfs_printf("Read PINOUT.TXT");
    return read_iw_sector(sector_offset, data, num_sectors, settings_build_pinout_txt);
}

void vfs_user_build_filesystem(void)
{
    dbg("Rebuilding VFS...");

    // Setup the filesystem based on target parameters
    vfs_init(daplink_drive_name, 0/*unused "disk size"*/);

    vfs_create_file("UNITS   INI", read_file_units_ini, NULL, iw_measure_total(settings_build_units_ini));
    vfs_create_file("SYSTEM  INI", read_file_system_ini, NULL, iw_measure_total(settings_build_system_ini));
    vfs_create_file("PINOUT  TXT", read_file_pinout_txt, NULL, iw_measure_total(settings_build_pinout_txt));
}

// Callback to handle changes to the root directory.  Should be used with vfs_set_file_change_callback
void vfs_user_file_change_handler(const vfs_filename_t filename,
                                  vfs_file_change_t change,
                                  vfs_file_t file, vfs_file_t new_file_data)
{
    // Check for "System Volume Information"
    if (strstarts(filename, "SYSTEM~1")) {
        vfs_printf("ATTN! Windows host detected.");
        // TODO take precautions if needed
        vfs_is_windows = true;
        return;
    }

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
