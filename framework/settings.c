//
// Created by MightyPork on 2017/11/26.
//

#include <comm/interfaces.h>
#include "platform.h"
#include "utils/hexdump.h"
#include "settings.h"
#include "unit_registry.h"
#include "system_settings.h"
#include "utils/str_utils.h"
#include "unit_base.h"
#include "platform/debug_uart.h"
#include "utils/avrlibc.h"

// pre-declarations
static void savebuf_flush(PayloadBuilder *pb, bool final);
static bool savebuf_ovhandler(PayloadBuilder *pb, uint32_t more);

// This is the first entry in a valid config.
// Change with each breaking change to force config reset.
#define CONFIG_MARKER 0xA55E

void settings_load(void)
{
    uint8_t *buffer = (uint8_t *) SETTINGS_FLASH_ADDR;

#if DEBUG_FLASH_WRITE
    dbg("reading @ %p", (void*)SETTINGS_FLASH_ADDR);
    hexDump("Flash", buffer, 64);
#endif

    PayloadParser pp = pp_start(buffer, SETTINGS_BLOCK_SIZE, NULL);

    // Check the integrity marker
    if (pp_u16(&pp) != CONFIG_MARKER) {
        dbg("Config not valid!");
        // Save for next run
        settings_save();
        return;
    }

    uint8_t version = pp_u8(&pp); // top level settings format version

    { // Settings
        (void)version; // Conditional choices based on version

        // System section
        if (!systemsettings_load(&pp)) {
            dbg("!! System settings failed to load");
            return;
        }

        if (!ureg_load_units(&pp)) {
            dbg("!! Unit settings failed to load");
            return;
        }
    } // End settings

    dbg("System settings loaded OK");
}


static uint8_t *save_buffer = NULL;
static uint32_t save_addr;

#if DEBUG_FLASH_WRITE
#define fls_printf(fmt, ...) PRINTF(fmt, ##__VA_ARGS__)
#else
#define fls_printf(fmt, ...) do {} while (0)
#endif

/**
 * Save buffer overflow handler.
 * This should flush whatever is in the buffer and let CWPack continue
 *
 * @param pb - buffer
 * @param more - how many more bytes are needed (this is meant for realloc / buffer expanding)
 * @return - success code
 */
static bool savebuf_ovhandler(PayloadBuilder *pb, uint32_t more)
{
    if (more > FLASH_SAVE_BUF_LEN) return false;
    savebuf_flush(pb, false);
    return true;
}

// Save settings to flash
void settings_save(void)
{
    HAL_StatusTypeDef hst;

    assert_param(save_buffer == NULL); // It must be NULL here - otherwise we have a leak
    save_buffer = malloc_ck(FLASH_SAVE_BUF_LEN);
    assert_param(save_buffer != NULL);

    PayloadBuilder pb = pb_start(save_buffer, FLASH_SAVE_BUF_LEN, savebuf_ovhandler);

    save_addr = SETTINGS_FLASH_ADDR;

    fls_printf("--- Starting flash write... ---\r\n");
    hst = HAL_FLASH_Unlock();
    assert_param(hst == HAL_OK);
    {
        fls_printf("ERASE flash pages for settings storage...\r\n");
        // We have to first erase the pages
        FLASH_EraseInitTypeDef erase;

#if PLAT_FLASHBANKS
        erase.Banks = FLASH_BANK_1; // TODO ?????
#endif

#if GEX_PLAT_F407
        // specialty for F4 with too much flash
        erase.NbSectors = 1;
        erase.Sector = SETTINGS_FLASH_SECTOR;
        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        erase.Banks = FLASH_BANK_1; // unused for sector erase
#else
        erase.NbPages = SETTINGS_BLOCK_SIZE/FLASH_PAGE_SIZE;
        erase.PageAddress = SETTINGS_FLASH_ADDR;
        erase.TypeErase = FLASH_TYPEERASE_PAGES;
#endif

        uint32_t pgerror = 0;
        hst = HAL_FLASHEx_Erase(&erase, &pgerror);
        assert_param(pgerror == 0xFFFFFFFFU);
        assert_param(hst == HAL_OK);

        // and now we can start writing...

        fls_printf("Beginning settings collect\r\n");

        // Marker that this is a valid save
        pb_u16(&pb, CONFIG_MARKER);
        pb_u8(&pb, 0); // Settings format version

        { // Settings
            fls_printf("Saving system settings\r\n");
            systemsettings_save(&pb);

            fls_printf("Saving units\r\n");
            ureg_save_units(&pb);
        } // End settings

        fls_printf("Final flush\r\n");
        savebuf_flush(&pb, true);
    }

    fls_printf("Locking flash...\r\n");
    hst = HAL_FLASH_Lock();
    assert_param(hst == HAL_OK);
    fls_printf("--- Flash done ---\r\n");

    free_ck(save_buffer);
    save_buffer = NULL;

#if DEBUG_FLASH_WRITE
    dbg("written @ %p", (void*)SETTINGS_FLASH_ADDR);
    hexDump("Flash", (void*)SETTINGS_FLASH_ADDR, 64);
#endif
}

/**
 * Flush the save buffer to flash, moving leftovers from uneven half-words
 * to the beginning and adjusting the CWPack curent pointer accordingly.
 *
 * @param ctx - pack context
 * @param final - if true, flush uneven leftovers; else move them to the beginning and keep for next call.
 */
static void savebuf_flush(PayloadBuilder *pb, bool final)
{
    assert_param(save_buffer != NULL);

    // TODO this might be buggy, was not tested cross-boundary yet
    // TODO remove those printf's after verifying correctness

    uint32_t bytes = (uint32_t) pb_length(pb);

    // Dump what we're flushing
    fls_printf("Flush: ");
    for (uint32_t i = 0; i < bytes; i++) {
        fls_printf("%02X ", save_buffer[i]);
    }
    fls_printf("\r\n");

    uint32_t halfwords = bytes >> 1;
    uint32_t remain = bytes & 1; // how many bytes won't be programmed

    fls_printf("Halfwords: %d, Remain: %d, last? %d\r\n", (int)halfwords, (int)remain, final);

    uint16_t *hwbuf = (void*) &save_buffer[0];

    for (; halfwords > 0; halfwords--) {
        uint16_t hword = *hwbuf++;

        fls_printf("%04X ", hword);

        HAL_StatusTypeDef res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, save_addr, hword);
        assert_param(HAL_OK == res);
        save_addr += 2; // advance
    }

    // rewind the context buffer
    pb->current = pb->start;

    if (remain) {
        // We have an odd byte left to write
        if (final) {
            // We're done writing, this is the last call. Append the last byte to flash.
            uint16_t hword = save_buffer[bytes-1];
            fls_printf("& %02X ", hword);

            HAL_StatusTypeDef res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, save_addr, hword);
            assert_param(HAL_OK == res);
        } else {
            // Move the leftover to the beginning of the buffer for next call.
            save_buffer[0] = save_buffer[bytes-1];
            pb->current++;
        }
    }
    fls_printf("\r\n");
}

// ---------------------------------------------------------------

/** Generate a file header */
static void gex_file_preamble(IniWriter *iw, const char *filename)
{
// File header
    iw_hdr_comment(iw, filename);
    iw_hdr_comment(iw, "GEX v%s on %s", GEX_VERSION, GEX_PLATFORM);
    iw_hdr_comment(iw, "built %s at %s", __DATE__, __TIME__);
}

/** Generate a config file header (write instructions) */
static void ini_preamble(IniWriter *iw, const char *filename)
{
    gex_file_preamble(iw, filename);

    if (iw->tag == 0) { // tag 1 is set when exporting via the API
        iw_cmt_newline(iw);
        iw_comment(iw, "Overwrite this file to change settings.");
        #if PLAT_LOCK_BTN
            iw_comment(iw, "Press the LOCK button to save them to Flash.");
        #else
            iw_comment(iw, "Close the LOCK jumper to save them to Flash.");
        #endif
    }
}

// --- UNITS.INI ---

extern osMutexId mutScratchBufferHandle;

/**
 * Build units ini file
 */
void settings_build_units_ini(IniWriter *iw)
{
    ini_preamble(iw, "UNITS.INI");
    assert_param(osOK == osMutexWait(mutScratchBufferHandle, 5000));
    {
        ureg_build_ini(iw);
    }
    assert_param(osOK == osMutexRelease(mutScratchBufferHandle));
}

// --- SYSTEM.INI ---

/**
 * Build system settings ini file
 */
void settings_build_system_ini(IniWriter *iw)
{
    ini_preamble(iw, "SYSTEM.INI");
    systemsettings_build_ini(iw);
}

// --- PINOUT.TXT ---

/** print system pin.-out info (platform.c) */
extern void plat_print_system_pinout(IniWriter *iw);

/**
 * Build pinout info file
 */
void settings_build_pinout_txt(IniWriter *iw)
{
    gex_file_preamble(iw, "PINOUT.TXT");
    iw_cmt_newline(iw);
    rsc_print_all_available(iw);
    ureg_print_unit_resources(iw);
    plat_print_system_pinout(iw);
}

// ---------------------------------------------------------------

void settings_load_ini_begin(void)
{
    SystemSettings.modified = true;
    SystemSettings.loading_inifile = 0;
}

void settings_load_ini_key(const char *restrict section, const char *restrict key, const char *restrict value)
{
//    dbg("[%s] %s = %s", section, key, value);
    char namebuf[INI_KEY_MAX];

    // SYSTEM and UNITS files must be separate.
    // Init functions are run for first key in the section.

    if (streq(section, "SYSTEM")) {
        if (SystemSettings.loading_inifile == 0) {
            SystemSettings.loading_inifile = 'S';
            systemsettings_begin_load();
            systemsettings_loadDefaults();
        }

        // system is always at the top
        systemsettings_load_ini(key, value);
    }
    else if (streq(section, "UNITS")) {

        if (SystemSettings.loading_inifile == 0) {
            SystemSettings.loading_inifile = 'U';
            ureg_remove_all_units();
        }

        // this will always come before individual units config
        // install or tear down units as described by the config
        ureg_instantiate_by_ini(key, value);
    }
    else {
        // not a standard section, may be some unit config
        // all unit sections contain the colon character [TYPE:NAME@CALLSIGN]
        const char *nameptr = strchr(section, ':');
        const char *csptr = strchr(section, '@');

        if (nameptr != NULL && csptr != NULL) {
            strncpy(namebuf, nameptr+1, csptr - nameptr - 1);
            namebuf[csptr - nameptr - 1] = 0;
            uint8_t cs = (uint8_t) avr_atoi(csptr + 1);

            error_t rv = ureg_load_unit_ini_key(namebuf, key, value, cs);
            if (rv != E_SUCCESS)
                dbg("!! error loading %s@%d.%s = %s - error %s", namebuf, (int)cs, key, value, error_get_message(rv));
        } else {
            dbg("! Bad config key: [%s] %s = %s", section, key, value);
        }
    }
}


void settings_load_ini_end(void)
{
    if (SystemSettings.loading_inifile == 'U') {
        bool suc = ureg_finalize_all_init();
        if (!suc) dbg("Some units failed to init!!");
    }

    if (SystemSettings.loading_inifile == 'S') {
        systemsettings_finalize_load();
    }
}
