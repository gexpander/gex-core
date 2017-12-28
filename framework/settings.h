//
// Created by MightyPork on 2017/11/26.
//

#ifndef GEX_SETTINGS_H
#define GEX_SETTINGS_H

#include "platform.h"
#include "utils/ini_writer.h"

/**
 * Load settings from flash (system settings + units).
 * Unit registry must be already initialized.
 *
 * This should happen only once, during the boot sequence.
 */
void settings_load(void);

/**
 * Save all settings to flash.
 * This may be called multiple times after user changes the config file.
 */
void settings_save(void);


/**
 * Call this before any of the ini read stuff
 *
 * Resets everything to defaults so we have a clean start.
 *
 * NOTE: Should the file be received only partially, this may corrupt the settings.
 * For this reason we don't commit it to flash immediately but require user to replace
 * the LOCK jumper before unplugging the device.
 */
void settings_load_ini_begin(void);

/**
 * Load settings from INI kv pair.
 */
void settings_load_ini_key(const char *restrict section, const char *restrict key, const char *restrict value);

/**
 * Call this before any of the ini read stuff
 *
 * Resets everything to defaults so we have a clean start.
 *
 * NOTE: Should the file be received only partially, this may corrupt the settings.
 * For this reason we don't commit it to flash immediately but require user to replace
 * the LOCK jumper before unplugging the device.
 */
void settings_load_ini_end(void);

/**
 * Write UNITS.INI to a iniwriter
 * @param iw - writer handle
 */
void settings_build_units_ini(IniWriter *iw);

/**
 * Get UNITS.INI len (expensive, uses dummy read)
 *
 * @return bytes
 */
uint32_t settings_get_units_ini_len(void);

/**
 * Write SYSTEM.INI to iniwriter
 * @param iw - writer handle
 */
void settings_build_system_ini(IniWriter *iw);

/**
 * Get SYSTEM.INI len (expensive, uses dummy read)
 *
 * @return bytes
 */
uint32_t settings_get_system_ini_len(void);

#endif //GEX_SETTINGS_H
