//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "comm/messages.h"
#include "framework/resources.h"
#include "framework/settings.h"
#include "framework/system_settings.h"

#include "lock_jumper.h"
#include "status_led.h"
#include "debug_uart.h"

void plat_init(void)
{
    // Load system defaults
    systemsettings_init();

    dbg("Setting up resources ...");
    rsc_init_registry();
    plat_init_resources(); // also registers unit drivers

    LockJumper_Init();
    StatusLed_Init();
    DebugUart_Init(); // <- only the resource claim

    dbg("Loading settings ...");
    // Load settings from Flash and apply (includes System settings and all Unit settings)
    settings_load(); // XXX maybe this should be moved to the main task

    comm_init();
}
