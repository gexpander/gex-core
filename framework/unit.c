//
// Created by MightyPork on 2017/11/24.
//

#include "platform.h"
#include "unit.h"
#include "resources.h"
#include "unit_base.h"

char unit_tmp512[UNIT_TMP_LEN];

// Abort partly inited unit
void clean_failed_unit(Unit *unit)
{
    if (unit == NULL) return;

    dbg("!! Init of [%s] failed!", unit->name);

    // Free if it looks like it might've been allocated
    free_ck(unit->data);
    free_ck(unit->name);

    dbg("Releasing any held resources");
    // Release any already claimed resources
    rsc_teardown(unit);
}

// ----------------------------------------------------

// system unit is used to claim peripherals on behalf of the system (e.g. HAL tick source)
Unit UNIT_SYSTEM = {
    .name = "SYSTEM"
};

// ----------------------------------------------------

// platform unit is used to claim peripherals not present on the current platform
Unit UNIT_PLATFORM = {
    .name = "PLATFORM"
};

// ----------------------------------------------------
