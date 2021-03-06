//
// Created by MightyPork on 2017/11/26.
//
// Initialize the platform. Belongs to platform.h
//

#include "platform.h"
#include "comm/messages.h"
#include "framework/resources.h"
#include "framework/settings.h"
#include "framework/system_settings.h"

#include "lock_jumper.h"
#include "status_led.h"
#include "debug_uart.h"
#include "irq_dispatcher.h"
#include "timebase.h"
#include "watchdog.h"

void plat_init(void)
{
    // GPIO clocks are enabled earlier in the GEX start-up hook

    // Load system defaults
    systemsettings_init();

    dbg("Setting up resources ...");
    rsc_init_registry();
    plat_init_resources(); // also registers unit drivers

    LockJumper_Init();
    Indicator_Init();
    DebugUart_Init(); // resource claim (was inited earlier to allow debug outputs)

    // Enable interrupts and set priorities
    irqd_init();

    dbg("Loading settings ...");
    // Load settings from Flash and apply (includes System settings and all Unit settings)
    settings_load(); // XXX maybe this should be moved to the main task

    comm_init();

    wd_init();
}
