//
// Created by MightyPork on 2018/02/01.
//

#ifndef GEX_F072_OW_SEARCH_H
#define GEX_F072_OW_SEARCH_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#include <stdint.h>
#include <stdbool.h>
#include "unit_base.h"

// --------------------------------------------------------------------------------------

/**
 * Data type holding a romcode
 */
typedef uint8_t ow_romcode_t[8];

/**
 * Get a single bit from a romcode
 */
#define ow_code_getbit(code, index) (bool)((code)[(index) >> 3] & (1 << ((index) & 7)))

/**
 * Convert to unsigned 64-bit integer
 * (works only on little-endian systems - eg. OK on x86/x86_64, not on PowerPC)
 */
#define ow_romcode_to_u64(code) (*((uint64_t *) (void *)(code)))

/**
 * States of the search algorithm
 */
enum ow_search_result {
    OW_SEARCH_DONE = 0,
    OW_SEARCH_MORE = 1,
    OW_SEARCH_FAILED = 2,
};

/**
 * Internal state of the search algorithm.
 * Check status to see if more remain to be read or an error occurred.
 */
struct ow_search_state {
    int8_t prev_last_fork;
    ow_romcode_t prev_code;
    uint8_t command;
    enum ow_search_result status;
    bool first;
    bool test_checksums;
    error_t error;
};

/**
 * Init the search algorithm state structure
 *
 * @param[out] state - inited struct
 * @param[in] command - command to send for requesting the search (e.g. SEARCH_ROM)
 * @param[in] test_checksums - verify checksums of all read romcodes
 */
void ow_search_init(Unit *unit, uint8_t command, bool test_checksums);

/**
 * Perform a search of the 1-wire bus, with a state struct pre-inited
 * using ow_search_init().
 *
 * Romcodes are stored in the provided array in a numerically ascending order.
 *
 * This function may be called repeatedly to retrieve more addresses than could fit
 * in the address buffer.
 *
 * @param[in,out] state - search state, used for multiple calls with limited buffer size
 * @param[out] codes - buffer for found romcodes
 * @param[in] capacity - buffer capacity
 * @return number of romcodes found. Search status is stored in `state->status`,
 *         possible error code in `status->error`
 */
uint32_t ow_search_run(Unit *unit, ow_romcode_t *codes, uint32_t capacity);

#endif //GEX_F072_OW_SEARCH_H
