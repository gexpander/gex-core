//
// Created by MightyPork on 2017/12/08.
//

#ifndef GEX_PLATFORM_H
#define GEX_PLATFORM_H

#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

// FreeRTOS includes
#include <cmsis_os.h>
// platform-specific stuff (includes stm32 driver headers)
#include "plat_compat.h"
// assert_param, trap...
#include "stm32_assert.h"
// inIRQ etc
#include "cortex_utils.h"
// MIN, MAX, static assert etc
#include "macro.h"
// smaller replacement for regular snprintf - SNPRINTF
#include "snprintf.h"
// debug logging
#include "debug.h"
// error codes and strings
#include "utils/error.h"
// GEX version string
#include "version.h"


// ---

/**
 * Init the platform (run before FreeRTOS starts)
 */
void plat_init(void);

/**
 * Init resources available for this platform
 */
void plat_init_resources(void);

/**
 * Register units available for this platform / build
 */
void plat_register_units(void);

/**
 * Re-connect the USB, triggering descriptors reload.
 * Use the DPPU bit on USB_BCDR, if available.
 */
void plat_usb_reconnect(void);

// provided as extern
//void plat_print_system_pinout(IniWriter *iw);

#endif //GEX_PLATFORM_H
