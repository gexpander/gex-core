//
// Created by MightyPork on 2018/02/27.
//

#include "platform.h"
#include "watchdog.h"

static volatile uint16_t suspend_depth = 0;
static volatile bool restart_pending = false;

void wd_init(void)
{
    dbg("IWDG init, time 2s");

    LL_IWDG_Enable(IWDG);
    LL_IWDG_EnableWriteAccess(IWDG);
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_32); // 0.8 ms
    LL_IWDG_SetReloadCounter(IWDG, 2500); // 2s.  max 4095
    while (!LL_IWDG_IsReady(IWDG));

    // reload
    LL_IWDG_ReloadCounter(IWDG);
}

void wd_suspend(void)
{
    vPortEnterCritical();
    if (suspend_depth < 0xFFFF) {
        suspend_depth++;
    }
    vPortExitCritical();
}

void wd_resume(void)
{
    vPortEnterCritical();
    if (suspend_depth > 0) {
        suspend_depth--;

        if (suspend_depth == 0 && restart_pending) {
            restart_pending = false;
            LL_IWDG_ReloadCounter(IWDG);
        }
    }
    vPortExitCritical();
}

void wd_restart(void)
{
    vPortEnterCritical();
    if (suspend_depth == 0) {
        LL_IWDG_ReloadCounter(IWDG);
    } else {
        restart_pending = true;
    }
    vPortExitCritical();
}
