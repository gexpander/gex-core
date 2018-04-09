//
// Created by MightyPork on 2017/12/02.
//
// SYSTEM.INI system settings
//

#ifndef GEX_SYSTEM_SETTINGS_H
#define GEX_SYSTEM_SETTINGS_H

#include "platform.h"
#include "utils/ini_writer.h"
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

/**
 * Struct of the global system settings storage
 */
struct system_settings {
    bool visible_vcom;
    bool ini_comments;
    bool enable_mco;
    uint8_t mco_prediv;
    bool enable_debug_uart;

    // enable alternate communication ports if USB doesn't enumerate (e.g. running from battery / solar cell remotely)
    bool use_comm_uart;
    uint32_t comm_uart_baud; // baud rate for the uart transport
    bool use_comm_lora;   // SX1276/8
    bool use_comm_nordic; // nRF24L01+
    uint8_t nrf_channel;
    uint8_t nrf_network[4];
    uint8_t nrf_address;

    // Support flags put here for scoping, but not atcually part of the persistent settings
    volatile bool editable; //!< True if we booted with the LOCK jumper removed
    volatile bool modified; //!< True if user did any change to the settings (checked when the LOCK jumper is replaced)
    volatile char loading_inifile; // S-system, U-units
};

/** Global system settings storage */
extern struct system_settings SystemSettings;

/**
 * Init the store
 */
void systemsettings_init(void);

/**
 * Load defaults
 */
void systemsettings_loadDefaults(void);

/**
 * Write system settings to the pack context
 */
void systemsettings_save(PayloadBuilder *pb);

/**
 * Load system settings from the unpack context
 */
bool systemsettings_load(PayloadParser *pp);

/**
 * Write system settings to INI
 */
void systemsettings_build_ini(IniWriter *iw);

/**
 * Load system settings from INI kv pair
 *
 * @return true on success
 */
bool systemsettings_load_ini(const char *restrict key, const char *restrict value);

/** Release system resources before system settings init */
void systemsettings_begin_load(void);
/** Claim system resources and apply system settings */
void systemsettings_finalize_load(void);

#endif //GEX_SYSTEM_SETTINGS_H
