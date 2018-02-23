//
// Created by MightyPork on 2017/11/24.
//
// Resource repository. Provides routines for claiming and releasing resources,
// resource listing, owner look-up etc.
//
// Resource is a peripheral or a software facility with exclusive access.
//
// Resources are assigned to:
// - PLATFORM if they're not available
// - SYSTEM if they're claimed internally (e.g. status LED pin)
// - individual user-defined units
//

#ifndef GEX_RESOURCES_H
#define GEX_RESOURCES_H

#include "platform.h"
#include "unit.h"
#include "rsc_enum.h"

#if DEBUG_RSC
  #define rsc_dbg(fmt, ...) dbg("[RSC] "fmt, ##__VA_ARGS__)
#else
  #define rsc_dbg(fmt, ...)
#endif

/**
 * Initialize the repository. Mark all resources as claimed by PLATFORM.
 */
void rsc_init_registry(void);

/**
 * Claim a GPIO using port name and pin number.
 *
 * @param unit - claiming unit
 * @param port_name - port (eg. 'A')
 * @param pin - pin number 0-15
 * @return success
 */
error_t rsc_claim_pin(Unit *unit, char port_name, uint8_t pin) __attribute__((warn_unused_result));

/**
 * Claim a resource by the Resource enum
 *
 * @param unit - claiming unit
 * @param rsc - resource to claim
 * @return success
 */
error_t rsc_claim(Unit *unit, Resource rsc) __attribute__((warn_unused_result));

/**
 * Claim a range of resources (use for resources of the same type, e.g. USART1-5)
 *
 * @param unit - claiming unit
 * @param rsc0 - first resource to claim
 * @param rsc1 - last resource to claim
 * @return success (E_SUCCESS = complete claim)
 */
error_t rsc_claim_range(Unit *unit, Resource rsc0, Resource rsc1) __attribute__((warn_unused_result));

/**
 * Claim GPIOs by bitmask and port name, atomically.
 * Tear down the unit on failure.
 *
 * @param unit - claiming unit
 * @param port_name - port (eg. 'A')
 * @param pins - pins, bitmask
 * @return success (E_SUCCESS = complete claim)
 */
error_t rsc_claim_gpios(Unit *unit, char port_name, uint16_t pins) __attribute__((warn_unused_result));

/**
 * Release all resources held by a unit, de-init held GPIOs
 *
 * @param unit - unit to tear down
 */
void rsc_teardown(Unit *unit);

/**
 * Release a resource by the Resource enum
 *
 * @param unit - holding unit, NULL to ignore
 * @param rsc - resource to release
 */
void rsc_free(Unit *unit, Resource rsc);

/**
 * Release a range of resources (use for resources of the same type, e.g. USART1-5)
 *
 * @param unit - releasing unit
 * @param rsc0 - first resource to release
 * @param rsc1 - last resource to release
 */
void rsc_free_range(Unit *unit, Resource rsc0, Resource rsc1);

/**
 * Convert pin name and number to a resource enum
 *
 * @param port_name - char 'A'..'Z'
 * @param pin_number - number 0..15
 * @param suc - set to false on failure, left unchanged on success
 * @return the resource, or R_NONE
 */
Resource rsc_portpin2rsc(char port_name, uint8_t pin_number, bool *suc);

/**
 * Get name of a resource by the Resource enum.
 * Uses a static buffer, DO NOT store the returned pointer.
 *
 * @param rsc - resource to inspect
 * @return resource name (eg. PA1)
 */
const char * rsc_get_name(Resource rsc);

/**
 * Get rsc owner name
 *
 * @param rsc - resource to inspect
 * @return name of the holding unit, or "NULL" (string)
 */
const char * rsc_get_owner_name(Resource rsc);

/**
 * Print all available resource names
 * (this is the pins part of the PINOUT.TXT file)
 *
 * @param iw - writer to use
 */
void rsc_print_all_available(IniWriter *iw);

#endif //GEX_RESOURCES_H
