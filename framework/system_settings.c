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

struct system_settings SystemSettings;

/** Load defaults only */
void systemsettings_loadDefaults(void)
{
    SystemSettings.visible_vcom = true;
    SystemSettings.ini_comments = true;
    SystemSettings.enable_mco = false;
    SystemSettings.mco_prediv = 7;
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
    pb_u8(pb, 1); // settings format version

    { // system settings
        pb_bool(pb, SystemSettings.visible_vcom);
        pb_bool(pb, SystemSettings.ini_comments);

        pb_bool(pb, SystemSettings.enable_mco);
        pb_u8(pb, SystemSettings.mco_prediv);
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

// from binary
bool systemsettings_load(PayloadParser *pp)
{
    if (pp_char(pp) != 'S') return false;

    systemsettings_mco_teardown();

    uint8_t version = pp_u8(pp);

    { // system settings
        SystemSettings.visible_vcom = pp_bool(pp);
        SystemSettings.ini_comments = pp_bool(pp);

        // conditional fields based on version
        if (version >= 1) {
            SystemSettings.enable_mco = pp_bool(pp);
            SystemSettings.mco_prediv = pp_u8(pp);
        }
    } // end system settings

    systemsettings_mco_init();

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

    iw_cmt_newline(iw);
    iw_comment(iw, "Output core clock on PA8 (Y, N)");
    iw_entry_s(iw, "mco-enable", str_yn(SystemSettings.enable_mco));
    iw_comment(iw, "Output clock prediv (1,2,...,128)");
    iw_entry_d(iw, "mco-prediv", (1<<SystemSettings.mco_prediv));
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

    return suc;
}
