//
// Created by MightyPork on 2017/11/24.
//

#include "platform.h"
#include "unit.h"
#include "resources.h"
#include "hw_utils.h"
#include "unit_registry.h"

static bool rsc_initialized = false;

static ResourceMap global_rscmap;

// here are the resource names for better debugging

// this list doesn't include GPIO and EXTI names, they can be easily generated
const char *const rsc_names[] = {
#define X(res_name) #res_name,
    XX_RESOURCES
#undef X
};

// Check that EXTI have higher values than GPIOs in the enum
// (determines the logic in the name generation code below)
COMPILER_ASSERT(R_EXTI0 > R_PA0);

/** Get rsc name */
const char * rsc_get_name(Resource rsc)
{
    assert_param(rsc < RESOURCE_COUNT);

    static char gpionamebuf[8];
    // we assume the returned value is not stored anywhere
    // and is directly used in a sprintf call, hence a static buffer is OK to use

    if (rsc >= R_EXTI0) {
        uint8_t index = rsc - R_EXTI0;
        SNPRINTF(gpionamebuf, 8, "EXTI%d", index);
        return gpionamebuf;
    }

    if (rsc >= R_PA0) {
        // we assume the returned value is not stored anywhere
        // and is directly used in a sprintf call.
        uint8_t index = rsc - R_PA0;
        SNPRINTF(gpionamebuf, 8, "P%c%d", 'A'+(index/16), index%16);
        return gpionamebuf;
    }

    return rsc_names[rsc];
}

/** Get rsc owner name */
const char * rsc_get_owner_name(Resource rsc)
{
    assert_param(rsc < RESOURCE_COUNT);

    Unit *pUnit = ureg_get_rsc_owner(rsc);
    if (pUnit == NULL) return "NULL";
    return pUnit->name;
}

/**
 * Initialize the resources registry
 */
void rsc_init_registry(void)
{
    for(uint32_t i = 0; i < RSCMAP_LEN; i++) {
        UNIT_PLATFORM.resources[i] = global_rscmap[i] = 0xFF;
    }

    rsc_initialized = true;
}

/**
 * Claim a resource for a unit
 *
 * @param unit - claiming unit
 * @param rsc - resource to claim
 * @return true on successful claim
 */
error_t rsc_claim(Unit *unit, Resource rsc)
{
    assert_param(rsc_initialized);
    assert_param(rsc < RESOURCE_COUNT);
    assert_param(unit != NULL);

    rsc_dbg("%s claims %s", unit->name, rsc_get_name(rsc));

    if (RSC_IS_HELD(global_rscmap, rsc)) {
        // this whole branch is just reporting the error

        Unit *holder = ureg_get_rsc_owner(rsc);
        assert_param(holder != NULL);

        dbg("ERROR!! Unit %s failed to claim rsc %s, already held by %s!",
            unit->name,
            rsc_get_name(rsc),
            holder->name);

        if (holder == unit) dbg("DOUBLE CLAIM, This is probably a bug!");

        unit->failed_rsc = rsc;

        return E_RESOURCE_NOT_AVAILABLE;
    }

    // must claim both in global and in unit
    RSC_CLAIM(global_rscmap, rsc);
    RSC_CLAIM(unit->resources, rsc);

    return E_SUCCESS;
}

/**
 * Claim a range of resources for a unit (useful for GPIO)
 *
 * @param unit - claiming unit
 * @param rsc0 - first resource to claim
 * @param rsc1 - last resource to claim
 * @return true on complete claim, false if any failed (none are claimed in that case)
 */
error_t rsc_claim_range(Unit *unit, Resource rsc0, Resource rsc1)
{
    assert_param(rsc_initialized);
    assert_param(rsc0 < RESOURCE_COUNT);
    assert_param(rsc1 < RESOURCE_COUNT);
    assert_param(rsc0 <= rsc1);
    assert_param(unit != NULL);

    for (int i = rsc0; i <= rsc1; i++) {
       TRY(rsc_claim(unit, (Resource) i));
    }

    return E_SUCCESS;
}

error_t rsc_claim_gpios(Unit *unit, char port_name, uint16_t pins)
{
    bool suc = true;

    for (int i = 0; i < 16; i++) {
        if (pins & (1 << i)) {
            Resource rsc = hw_pin2resource(port_name, (uint8_t) i, &suc);
            if (!suc) return E_BAD_CONFIG;

            TRY(rsc_claim(unit, rsc));
        }
    }
    return E_SUCCESS;
}

error_t rsc_claim_pin(Unit *unit, char port_name, uint8_t pin)
{
    bool suc = true;
    Resource rsc = hw_pin2resource(port_name, pin, &suc);
    if (!suc) return E_BAD_CONFIG;
    TRY(rsc_claim(unit, rsc));
    return E_SUCCESS;
}

/**
 * Free a resource for other use
 *
 * @param unit - owning unit; if not null, free only resources claimed by this unit
 * @param rsc - resource to free
 */
void rsc_free(Unit *unit, Resource rsc)
{
    assert_param(rsc_initialized);
    assert_param(rsc < RESOURCE_COUNT);

    rsc_dbg("Free resource %s", rsc_get_name(rsc));

    if (RSC_IS_FREE(global_rscmap, rsc)) return;

    // free it in any unit that holds it
    if (unit) {
        if (RSC_IS_HELD(unit->resources, rsc)) {
            RSC_FREE(unit->resources, rsc);
        }
    } else {
        // Try to free it in any unit that may hold it
        unit = ureg_get_rsc_owner(rsc);
        if (unit == NULL) {
            // try one of the built-in ones
            if (RSC_IS_HELD(UNIT_SYSTEM.resources, rsc)) unit = &UNIT_SYSTEM;
            else if (RSC_IS_HELD(UNIT_PLATFORM.resources, rsc)) unit = &UNIT_PLATFORM;
        }

        if (unit != NULL) RSC_FREE(unit->resources, rsc);
    }

    // also free it in the global map
    RSC_FREE(global_rscmap, rsc);
}

/**
 * Free a range of resources (useful for GPIO)
 *
 * @param unit - owning unit; if not null, free only resources claimed by this unit
 * @param rsc0 - first resource to free
 * @param rsc1 - last resource to free
 */
void rsc_free_range(Unit *unit, Resource rsc0, Resource rsc1)
{
    assert_param(rsc_initialized);
    assert_param(rsc0 < RESOURCE_COUNT);
    assert_param(rsc1 < RESOURCE_COUNT);
    assert_param(rsc0 <= rsc1);

    for (int i = rsc0; i <= rsc1; i++) {
        rsc_free(unit, (Resource) i);
    }
}

/**
 * Tear down a unit - release all resources owned by the unit.
 * Also de-init all GPIOs
 *
 * @param unit - unit to tear down; free only resources claimed by this unit
 */
void rsc_teardown(Unit *unit)
{
    assert_param(rsc_initialized);
    assert_param(unit != NULL);

    rsc_dbg("Tearing down unit %s", unit->name);
    hw_deinit_unit_pins(unit);

    for (uint32_t i = 0; i < RSCMAP_LEN; i++) {
        global_rscmap[i] &= ~unit->resources[i];
        unit->resources[i] = 0;
    }
}

void rsc_print_all_available(IniWriter *iw)
{
    if (iw->count == 0) return;

    iw_string(iw, "Resources available on this platform\r\n"
                  "------------------------------------\r\n");

    ResourceMap scratchmap = {};
    for (uint32_t i = 0; i < RSCMAP_LEN; i++) {
        scratchmap[i] = UNIT_PLATFORM.resources[i] | UNIT_SYSTEM.resources[i];
    }

    uint32_t count0 = (iw->count + iw->skip);
    bool first = true;
    for (uint32_t rsc = 0; rsc < R_PA0; rsc++) {
        if (RSC_IS_HELD(scratchmap, (Resource)rsc)) continue;
        if (!first) iw_string(iw, ", ");

        // automatic newlines
        if (count0 - (iw->count + iw->skip) >= 55) {
            iw_newline(iw);
            count0 = (iw->count + iw->skip);
        }

        iw_string(iw, rsc_get_name((Resource) rsc));
        first = false;
    }

    // GPIOs will be printed using the range format
    uint16_t bitmap = 0;
    for (uint32_t rsc = R_PA0, i = 0; rsc <= R_PF15; rsc++, i++) {
        if (i%16 == 0) {
            // here we print the previous port
            if (bitmap != 0) {
                iw_string(iw, pinmask2str(bitmap, iwbuffer));
                bitmap = 0;
            }

            // first of a port
            iw_newline(iw);
            iw_sprintf(iw, "P%c: ", (char)('A' + (i/16)));
        }

        if (RSC_IS_FREE(scratchmap, (Resource)rsc)) {
            bitmap |= 1<<i%16;
        }
    }
    // the last one
    if (bitmap != 0) {
        iw_string(iw, pinmask2str(bitmap, iwbuffer));
    }
    iw_newline(iw);
    iw_newline(iw);
}
