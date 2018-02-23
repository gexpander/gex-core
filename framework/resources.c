//
// Created by MightyPork on 2017/11/24.
//

#include "platform.h"
#include "unit.h"
#include "resources.h"
#include "hw_utils.h"
#include "cfg_utils.h"
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

COMPILER_ASSERT(R_PF15 < R_EXTI0);
COMPILER_ASSERT(R_PA0 == 0);

const char * rsc_get_name(Resource rsc)
{
    assert_param(rsc < RESOURCE_COUNT);

    static char gpionamebuf[8];
    // we assume the returned value is not stored anywhere
    // and is directly used in a sprintf call, hence a static buffer is OK to use

    if (rsc >= R_EXTI0 && rsc <= R_EXTI15) {
        uint8_t index = rsc - R_EXTI0;
        SNPRINTF(gpionamebuf, 8, "EXTI%d", index);
        return gpionamebuf;
    }

    // R_PA0 is 0
    if (rsc <= R_PF15) {
        // we assume the returned value is not stored anywhere
        // and is directly used in a sprintf call.
        uint8_t index = rsc;
        SNPRINTF(gpionamebuf, 8, "P%c%d", 'A'+(index/16), index%16);
        return gpionamebuf;
    }

    return rsc_names[rsc - R_EXTI15 - 1];
}

/** Convert a pin to resource handle */
Resource rsc_portpin2rsc(char port_name, uint8_t pin_number, bool *suc)
{
    assert_param(suc != NULL);

    if(port_name < 'A' || port_name >= ('A'+PORTS_COUNT)) {
        dbg("Bad port: %c", port_name); // TODO proper report
        *suc = false;
        return R_NONE;
    }

    if(pin_number > 15) {
        dbg("Bad pin: %d", pin_number); // TODO proper report
        *suc = false;
        return R_NONE;
    }

    uint8_t num = (uint8_t) (port_name - 'A');

    return R_PA0 + num*16 + pin_number;
}

const char * rsc_get_owner_name(Resource rsc)
{
    assert_param(rsc < RESOURCE_COUNT);

    Unit *pUnit = ureg_get_rsc_owner(rsc);
    if (pUnit == NULL) return "NULL";
    return pUnit->name;
}


void rsc_init_registry(void)
{
    for(uint32_t i = 0; i < RSCMAP_LEN; i++) {
        UNIT_PLATFORM.resources[i] = global_rscmap[i] = 0xFF;
    }

    rsc_initialized = true;
}


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
    for (int i = 0; i < 16; i++) {
        if (pins & (1 << i)) {
            TRY(rsc_claim_pin(unit, port_name, (uint8_t)i));
        }
    }
    return E_SUCCESS;
}


error_t rsc_claim_pin(Unit *unit, char port_name, uint8_t pin)
{
    bool suc = true;
    Resource rsc = rsc_portpin2rsc(port_name, pin, &suc);
    if (!suc) return E_BAD_CONFIG;
    TRY(rsc_claim(unit, rsc));
    return E_SUCCESS;
}


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


// build the pins part of the PINOUT.TXT file
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
    for (uint32_t rsc = R_EXTI15+1; rsc < R_NONE; rsc++) {
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
                iw_string(iw, cfg_pinmask_encode(bitmap, iwbuffer, 0));
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
        iw_string(iw, cfg_pinmask_encode(bitmap, iwbuffer, 0));
    }
    iw_newline(iw);
    iw_newline(iw);
}
