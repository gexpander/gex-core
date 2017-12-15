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
    plat_init_resources();

    LockJumper_Init();
    StatusLed_Init();
    DebugUart_Init(); // <- only the resource claim

    dbg("Registering platform units ...");
    // All user-configurable units are now added to the repository
    plat_register_units();

    dbg("Loading settings ...");
    // Load settings from Flash and apply (includes System settings and all Unit settings)
    settings_load();

    comm_init();
}
