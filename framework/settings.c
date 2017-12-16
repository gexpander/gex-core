//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "settings.h"
#include "unit_registry.h"
#include "system_settings.h"
#include "utils/str_utils.h"

// This is the first entry in a valid config.
// Change with each breaking change to force config reset.
#define CONFIG_MARKER 0xA55C

void settings_load(void)
{
    uint8_t *buffer = (uint8_t *) SETTINGS_FLASH_ADDR;

    PayloadParser pp = pp_start(buffer, SETTINGS_BLOCK_SIZE, NULL);

    // Check the integrity marker
    if (pp_u16(&pp) != CONFIG_MARKER) {
        dbg("Config not valid!");
        // Save for next run
        settings_save();
        return;
    }

    // System section
    if (!systemsettings_load(&pp)) {
        dbg("!! System settings failed to load");
        return;
    }

    if (!ureg_load_units(&pp)) {
        dbg("!! Unit settings failed to load");
        return;
    }

    dbg("System settings loaded OK");
}


#define SAVE_BUF_SIZE 256
static uint8_t save_buffer[SAVE_BUF_SIZE];
static uint32_t save_addr;

#if DEBUG_FLASH_WRITE
#define fls_printf(fmt, ...) dbg(fmt, ##__VA_ARGS__)
#else
#define fls_printf(fmt, ...) do {} while (0)
#endif

/**
 * Flush the save buffer to flash, moving leftovers from uneven half-words
 * to the beginning and adjusting the CWPack curent pointer accordingly.
 *
 * @param ctx - pack context
 * @param final - if true, flush uneven leftovers; else move them to the beginning and keep for next call.
 */
static void savebuf_flush(PayloadBuilder *pb, bool final)
{
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
    if (more > SAVE_BUF_SIZE) return false;
    savebuf_flush(pb, false);
    return true;
}

// Save settings to flash
void settings_save(void)
{
    HAL_StatusTypeDef hst;
    PayloadBuilder pb = pb_start(save_buffer, SAVE_BUF_SIZE, savebuf_ovhandler);

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

        erase.NbPages = SETTINGS_BLOCK_SIZE/FLASH_PAGE_SIZE;
        erase.PageAddress = SETTINGS_FLASH_ADDR;
        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        uint32_t pgerror = 0;
        hst = HAL_FLASHEx_Erase(&erase, &pgerror);
        assert_param(pgerror == 0xFFFFFFFFU);
        assert_param(hst == HAL_OK);

        // and now we can start writing...

        fls_printf("Beginning settings collect\r\n");

        // Marker that this is a valid save
        pb_u16(&pb, CONFIG_MARKER);
        fls_printf("Saving system settings\r\n");
        systemsettings_save(&pb);
        fls_printf("Saving units\r\n");
        ureg_save_units(&pb);

        fls_printf("Final flush\r\n");
        savebuf_flush(&pb, true);
    }
    fls_printf("Locking flash...\r\n");
    hst = HAL_FLASH_Lock();
    assert_param(hst == HAL_OK);
    fls_printf("--- Flash done ---\r\n");
}

/**
 * Write system settings to INI (without section)
 */
void settings_write_ini(IniWriter *iw)
{
    // File header
    iw_comment(iw, "CONFIG.INI");
    iw_comment(iw, "Changes are applied on file save and can be immediately tested and verified.");
    iw_comment(iw, "To persist to flash, replace the LOCK jumper before disconnecting from USB."); // TODO the jumper...

    systemsettings_write_ini(iw);
    iw_newline(iw);

    ureg_export_combined(iw);
}


void settings_read_ini_begin(void)
{
    SystemSettings.modified = true;

    // load defaults
    systemsettings_init();
    ureg_remove_all_units();
}

void settings_read_ini(const char *restrict section, const char *restrict key, const char *restrict value)
{
//    dbg("[%s] %s = %s", section, key, value);

    if (streq(section, "SYSTEM")) {
        // system is always at the top
        systemsettings_read_ini(key, value);
    }
    else if (streq(section, "UNITS")) {
        // this will always come before individual units config
        // install or tear down units as described by the config
        ureg_instantiate_by_ini(key, value);
    } else {
        // not a standard section, may be some unit config
        // all unit sections contain the colon character [TYPE:NAME]
        const char *nameptr = strchr(section, ':');
        if (nameptr) {
            ureg_read_unit_ini(nameptr+1, key, value);
        } else {
            dbg("! Bad config key: [%s] %s = %s", section, key, value);
        }
    }
}

void settings_read_ini_end(void)
{
    if (!ureg_finalize_all_init()) {
        dbg("Some units failed to init!!");
    }
}

uint32_t settings_get_ini_len(void)
{
    // this writer is configured to skip everything, so each written byte will decrement the skip count
    IniWriter iw = iw_init(NULL, 0xFFFFFFFF, 1);
    settings_write_ini(&iw);
    // now we just check how many bytes were skipped
    return 0xFFFFFFFF - iw.skip;
}
