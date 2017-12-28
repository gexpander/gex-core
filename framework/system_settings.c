//
// Created by MightyPork on 2017/12/02.
//

#include "platform.h"
#include "system_settings.h"
#include "utils/str_utils.h"
#include "platform/lock_jumper.h"

struct system_settings SystemSettings;

/** Load defaults only */
void systemsettings_loadDefaults(void)
{
    SystemSettings.visible_vcom = true;
    SystemSettings.ini_comments = true;
}

/** Load defaults and init flags */
void systemsettings_init(void)
{
    systemsettings_loadDefaults();

    // Flags
    SystemSettings.modified = false;
    LockJumper_ReadHardware();
}

// to binary
void systemsettings_save(PayloadBuilder *pb)
{
    pb_char(pb, 'S');
    pb_u8(pb, 0); // settings format version

    { // system settings
        pb_bool(pb, SystemSettings.visible_vcom);
        pb_bool(pb, SystemSettings.ini_comments);
    } // end system settings
}

// from binary
bool systemsettings_load(PayloadParser *pp)
{
    if (pp_char(pp) != 'S') return false;
    uint8_t version = pp_u8(pp);

    { // system settings
        SystemSettings.visible_vcom = pp_bool(pp);
        SystemSettings.ini_comments = pp_bool(pp);

        // conditional fields based on version
        (void) version;
    } // end system settings

    return pp->ok;
}


/**
 * Write system settings to INI (without section)
 */
void systemsettings_build_ini(IniWriter *iw)
{
    iw_section(iw, "SYSTEM");

    iw_comment(iw, "Data link accessible as virtual comport (Y, N)");
    iw_entry(iw, "expose_vcom", str_yn(SystemSettings.visible_vcom));

    iw_comment(iw, "Show comments in INI files (Y, N)");
    iw_entry(iw, "ini_comments", str_yn(SystemSettings.ini_comments));
}

/**
 * Load system settings from INI kv pair
 */
bool systemsettings_load_ini(const char *restrict key, const char *restrict value)
{
    bool suc = true;
    if (streq(key, "expose_vcom")) {
        bool yn = str_parse_yn(value, &suc);
        if (suc) SystemSettings.visible_vcom = yn;
    }

    if (streq(key, "ini_comments")) {
        bool yn = str_parse_yn(value, &suc);
        if (suc) SystemSettings.ini_comments = yn;
    }

    return suc;
}
