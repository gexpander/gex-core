//
// Created by MightyPork on 2017/12/02.
//

#include "platform.h"
#include "system_settings.h"
#include "utils/str_utils.h"
#include "platform/lock_jumper.h"
#include "cfg_utils.h"
#include "resources.h"
#include "unit_base.h"
#include "platform/debug_uart.h"
#include "comm/interfaces.h"

static void systemsettings_mco_teardown(void);
static void systemsettings_mco_init(void);
/** Init/deinit debug uart */
static void systemsettings_debug_uart_init_deinit(void);

struct system_settings SystemSettings;

/** Load defaults only */
void systemsettings_loadDefaults(void)
{
    SystemSettings.visible_vcom = true;
    SystemSettings.ini_comments = true;
    SystemSettings.enable_mco = false;
    SystemSettings.mco_prediv = 7;

    SystemSettings.use_comm_uart = false; // TODO configure those based on compile flags for a particular platform
    SystemSettings.use_comm_lora = false;
    SystemSettings.use_comm_nordic = false;
    SystemSettings.comm_uart_baud = 115200; // TODO

    SystemSettings.enable_debug_uart = true;
}

/** Load defaults and init flags */
void systemsettings_init(void)
{
    systemsettings_loadDefaults();

    // Flags
    SystemSettings.modified = false;
    LockJumper_CheckInitialState();
}

// to binary
void systemsettings_save(PayloadBuilder *pb)
{
    pb_char(pb, 'S');
    pb_u8(pb, 2); // settings format version

    { // system settings
        pb_bool(pb, SystemSettings.visible_vcom);
        pb_bool(pb, SystemSettings.ini_comments);
        // 1
        pb_bool(pb, SystemSettings.enable_mco);
        pb_u8(pb, SystemSettings.mco_prediv);
        // 2
        pb_bool(pb, SystemSettings.use_comm_uart);
        pb_bool(pb, SystemSettings.use_comm_nordic);
        pb_bool(pb, SystemSettings.use_comm_lora);
        pb_u32(pb, SystemSettings.comm_uart_baud);
        pb_bool(pb, SystemSettings.enable_debug_uart);
    } // end system settings
}

void systemsettings_mco_teardown(void)
{
    if (SystemSettings.enable_mco) {
        rsc_free(&UNIT_SYSTEM, R_PA8);
        LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_NOCLOCK, 0);
    }
}

void systemsettings_mco_init(void)
{
    if (SystemSettings.enable_mco) {
        assert_param(rsc_claim(&UNIT_SYSTEM, R_PA8) == E_SUCCESS);

        assert_param(E_SUCCESS == hw_configure_gpiorsc_af(R_PA8, LL_GPIO_AF_0));
        LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_SYSCLK,
                         SystemSettings.mco_prediv << RCC_CFGR_MCOPRE_Pos);
    } else {
        LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_NOCLOCK, 0);
    }
}

void systemsettings_debug_uart_init_deinit(void)
{
    if (SystemSettings.enable_debug_uart) {
        DebugUart_Init();
    } else {
        DebugUart_Teardown();
    }
}

/**
 * Begin load of system settings, releasing resources etc
 */
void systemsettings_begin_load(void)
{
    systemsettings_mco_teardown();
    com_release_resources_for_alt_transfers();
}

/**
 * Claim resources and set up system components based on the loaded settings
 */
void systemsettings_finalize_load(void)
{
    systemsettings_mco_init();
    systemsettings_debug_uart_init_deinit();
    com_claim_resources_for_alt_transfers();
}

// from binary
bool systemsettings_load(PayloadParser *pp)
{
    if (pp_char(pp) != 'S') return false;

    systemsettings_begin_load();

    uint8_t version = pp_u8(pp);

    { // system settings
        SystemSettings.visible_vcom = pp_bool(pp);
        SystemSettings.ini_comments = pp_bool(pp);

        // conditional fields based on version
        if (version >= 1) {
            SystemSettings.enable_mco = pp_bool(pp);
            SystemSettings.mco_prediv = pp_u8(pp);
        }
        if (version >= 2) {
            SystemSettings.use_comm_uart = pp_bool(pp);
            SystemSettings.use_comm_nordic = pp_bool(pp);
            SystemSettings.use_comm_lora = pp_bool(pp);
            SystemSettings.comm_uart_baud = pp_u32(pp);
            SystemSettings.enable_debug_uart = pp_bool(pp);
        }
    } // end system settings

    systemsettings_finalize_load();

    return pp->ok;
}


/**
 * Write system settings to INI (without section)
 */
void systemsettings_build_ini(IniWriter *iw)
{
    iw_section(iw, "SYSTEM");

    iw_comment(iw, "Data link accessible as virtual comport (Y, N)");
    iw_entry_s(iw, "expose-vcom", str_yn(SystemSettings.visible_vcom));

    iw_comment(iw, "Show comments in INI files (Y, N)");
    iw_entry_s(iw, "ini-comments", str_yn(SystemSettings.ini_comments));

    iw_comment(iw, "Enable debug UART-Tx on PA9 (Y, N)"); // TODO update if moved to a different pin
    iw_entry_s(iw, "debug-uart", str_yn(SystemSettings.enable_debug_uart));

    iw_cmt_newline(iw);
    iw_comment(iw, "Output core clock on PA8 (Y, N)");
    iw_entry_s(iw, "mco-enable", str_yn(SystemSettings.enable_mco));
    iw_comment(iw, "Output clock prediv (1,2,...,128)");
    iw_entry_d(iw, "mco-prediv", (1<<SystemSettings.mco_prediv));

    iw_cmt_newline(iw);
    iw_comment(iw, "--- Allowed fallback communication ports ---");

    iw_cmt_newline(iw);
    iw_comment(iw, "UART Tx:PA2, Rx:PA3");
    iw_entry_s(iw, "com-uart", str_yn(SystemSettings.use_comm_uart));
    iw_entry_d(iw, "com-uart-baud", SystemSettings.comm_uart_baud);

    iw_cmt_newline(iw);
    iw_comment(iw, "nRF24L01+ radio");
    iw_entry_s(iw, "com-nrf", str_yn(SystemSettings.use_comm_nordic));

    iw_comment(iw, "nRF channel");
    iw_entry_d(iw, "nrf-channel", SystemSettings.nrf_channel);

    iw_comment(iw, "nRF network ID (hex, 4 bytes, little-endian)");
    iw_entry(iw, "nrf-network", "%02X.%02X.%02X.%02X",
             SystemSettings.nrf_network[0],
             SystemSettings.nrf_network[1],
             SystemSettings.nrf_network[2],
             SystemSettings.nrf_network[3]);

    iw_comment(iw, "nRF node address (hex, 1 byte, > 0)");
    iw_entry(iw, "nrf-address", "%02X",
             SystemSettings.nrf_address);

    // those aren't implement yet, don't tease the user
    // TODO show pin-out, extra settings if applicable
#if 0
    iw_comment(iw, "LoRa/GFSK sx127x");
    iw_entry_s(iw, "com-lora", str_yn(SystemSettings.use_comm_sx127x));
#endif
}

/**
 * Load system settings from INI kv pair
 */
bool systemsettings_load_ini(const char *restrict key, const char *restrict value)
{
    bool suc = true;
    if (streq(key, "expose-vcom")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.visible_vcom = yn;
    }

    if (streq(key, "ini-comments")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.ini_comments = yn;
    }

    if (streq(key, "mco-enable")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.enable_mco = yn;
    }

    if (streq(key, "mco-prediv")) {
        int val = cfg_u8_parse(value, &suc);
        if (suc) {
            switch (val) {
                case 1: SystemSettings.mco_prediv = 0; break;
                case 2: SystemSettings.mco_prediv = 1; break;
                case 4: SystemSettings.mco_prediv = 2; break;
                case 8: SystemSettings.mco_prediv = 3; break;
                case 16: SystemSettings.mco_prediv = 4; break;
                case 32: SystemSettings.mco_prediv = 5; break;
                case 64: SystemSettings.mco_prediv = 6; break;
                default:
                case 128: SystemSettings.mco_prediv = 7; break;
            }
        }
    }

    if (streq(key, "debug-uart")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.enable_debug_uart = yn;
    }

    if (streq(key, "com-uart")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.use_comm_uart = yn;
    }

    if (streq(key, "com-uart-baud")) {
        uint32_t baud = cfg_u32_parse(value, &suc);
        if (suc) SystemSettings.comm_uart_baud = baud;
    }

    if (streq(key, "com-nrf")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.use_comm_nordic = yn;
    }

    if (streq(key, "nrf-channel")) {
        SystemSettings.nrf_channel = cfg_u8_parse(value, &suc);
    }

    if (streq(key, "nrf-address")) {
        cfg_hex_parse(&SystemSettings.nrf_address, 1, value, &suc);
    }

    if (streq(key, "nrf-network")) {
        cfg_hex_parse(&SystemSettings.nrf_network[0], 4, value, &suc);
    }

#if 0
    if (streq(key, "com-lora")) {
        bool yn = cfg_bool_parse(value, &suc);
        if (suc) SystemSettings.use_comm_lora = yn;
    }
#endif

    return suc;
}
