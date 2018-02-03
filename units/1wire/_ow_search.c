//
// Created by MightyPork on 2018/02/01.
//

#include "platform.h"
#include "unit_1wire.h"

#define OW_INTERNAL
#include "_ow_search.h"
#include "_ow_internal.h"
#include "_ow_low_level.h"
#include "_ow_commands.h"

void ow_search_init(Unit *unit, uint8_t command, bool test_checksums)
{
    if (unit->driver != &UNIT_1WIRE)
        trap("Wrong unit type - %s", unit->driver->name);

    assert_param(command == OW_ROM_SEARCH || command == OW_ROM_ALM_SEARCH);

    struct priv *priv = unit->data;
    struct ow_search_state *state = &priv->searchState;
    
    state->prev_last_fork = 64;
    memset(state->prev_code, 0, 8);
    state->status = OW_SEARCH_MORE;
    state->error = E_SUCCESS;
    state->command = command;
    state->first = true;
    state->test_checksums = test_checksums;
}

uint32_t ow_search_run(Unit *unit, ow_romcode_t *codes, uint32_t capacity)
{
    if (unit->driver != &UNIT_1WIRE)
        trap("Wrong unit type - %s", unit->driver->name);

    assert_param(codes);

    struct priv *priv = unit->data;
    struct ow_search_state *state = &priv->searchState;

    if (state->status != OW_SEARCH_MORE) return 0;

    uint32_t found_devices = 0;

    while (found_devices < capacity) {
        uint8_t index = 0;
        ow_romcode_t code = {};
        int8_t last_fork = -1;

        // Start a new transaction. Devices respond to reset
        if (!ow_reset(unit)) {
            state->status = OW_SEARCH_FAILED;
            state->error = E_HW_TIMEOUT;
            goto done;
        }
        // Send the search command (SEARCH_ROM, SEARCH_ALARM)
        ow_write_u8(unit, state->command);

        uint8_t *code_byte = &code[0];

        bool p, n;
        while (index != 64) {
            // Read a bit and its complement
            p = ow_read_bit(unit);
            n = ow_read_bit(unit);

            if (!p && !n) {
                // A fork: there are devices on the bus with different bit value
                // (the bus is open-drain, in both cases one device pulls it low)
                if ((found_devices > 0 || !state->first) && index < state->prev_last_fork) {
                    // earlier than the last fork, take the same turn as before
                    p = ow_code_getbit(state->prev_code, index);
                    if (!p) last_fork = index; // remember for future runs, 1 not explored yet
                }
                else if (index == state->prev_last_fork) {
                    p = 1; // both forks are now exhausted
                }
                else { // a new fork
                    last_fork = index;
                }
            }
            else if (p && n) {
                // No devices left connected - this doesn't normally happen
                state->status = OW_SEARCH_FAILED;
                state->error = E_BUS_FAULT;
                goto done;
            }

            // All devices have a matching bit here, or it was resolved in a fork
            if (p) *code_byte |= (1 << (index & 7));
            ow_write_bit(unit, p);

            index++;
            if((index & 7) == 0) {
                code_byte++;
            }
        }

        memcpy(state->prev_code, code, 8);

        if (state->test_checksums) {
            if (0 != ow_checksum(code, 8)) {
                state->status = OW_SEARCH_FAILED;
                state->error = E_CHECKSUM_MISMATCH;
                goto done;
            }
        }

        // Record a found address
        memcpy(codes[found_devices], code, 8);
        found_devices++;

        // Stop condition
        if (last_fork == -1) {
            state->status = OW_SEARCH_DONE;
            goto done;
        }

        state->prev_last_fork = last_fork;
    }

done:
    state->first = false;
    return found_devices;
}


