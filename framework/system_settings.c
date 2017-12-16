//
// Created by MightyPork on 2017/12/02.
//

#include "platform.h"
#include "utils/str_utils.h"
#include "system_settings.h"
#include "platform/lock_jumper.h"

struct system_settings SystemSettings;

void systemsettings_init(void)
{
    SystemSettings.visible_vcom = true;
    SystemSettings.modified = false;

    LockJumper_ReadHardware();
}

// to binary
void systemsettings_save(PayloadBuilder *pb)
{
    pb_char(pb, 'S');
    pb_bool(pb, SystemSettings.visible_vcom);
}

// from binary
bool systemsettings_load(PayloadParser *pp)
{
    if (pp_char(pp) != 'S') return false;
    SystemSettings.visible_vcom = pp_bool(pp);

    return pp->ok;
}


/**
 * Write system settings to INI (without section)
 */
void systemsettings_write_ini(IniWriter *iw)
{
    iw_section(iw, "SYSTEM");
    iw_comment(iw, "Expose the comm. channel as a virtual comport (Y, N)");
    iw_entry(iw, "expose_vcom", str_yn(SystemSettings.visible_vcom));
}

/**
 * Load system settings from INI kv pair
 */
bool systemsettings_read_ini(const char *restrict key, const char *restrict value)
{
    bool suc = true;
    if (streq(key, "expose_vcom")) {
        bool yn = str_parse_yn(value, &suc);
        if (suc) SystemSettings.visible_vcom = yn;
    }

    return suc;
}
