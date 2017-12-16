//
// Created by MightyPork on 2017/12/15.
//

#include "sched_queue.h"
#include "task_sched.h"
#include "usbd_core.h"
#include "usb_device.h"

#include "framework/settings.h"
#include "framework/resources.h"
#include "framework/system_settings.h"
#include "pin_utils.h"
#include "lock_jumper.h"

static GPIO_TypeDef *lock_periph;
static uint32_t lock_llpin;

// We use macros LOCK_JUMPER_PORT, LOCK_JUMPER_PIN from plat_compat.h

/** Init the jumper subsystem */
void LockJumper_Init(void)
{
    bool suc = true;

    // Resolve and claim resource
    Resource rsc = plat_pin2resource(LOCK_JUMPER_PORT, LOCK_JUMPER_PIN, &suc);
    assert_param(suc);

    suc &= rsc_claim(&UNIT_SYSTEM, rsc);
    assert_param(suc);

    // Resolve pin
    lock_periph = plat_port2periph(LOCK_JUMPER_PORT, &suc);
    lock_llpin = plat_pin2ll(LOCK_JUMPER_PIN, &suc);
    assert_param(suc);

    // Configure for input
    LL_GPIO_SetPinMode(lock_periph, lock_llpin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(lock_periph, lock_llpin, LL_GPIO_PULL_UP);

    SystemSettings.editable = (bool) LL_GPIO_IsInputPinSet(lock_periph, lock_llpin);
    dbg("Settings editable? %d", SystemSettings.editable);
}


/** Handle jumper state change */
static void jumper_changed(void)
{
    if (SystemSettings.editable) {
        // Unlock
        dbg("LOCK jumper removed, enabling MSC!");
    } else {
        // Lock
        dbg("LOCK jumper replaced, saving to Flash & disabling MSC!");

        if (SystemSettings.modified) {
            settings_save();
        }
    }

    plat_usb_reconnect();
}


/** Periodic jumper check */
void LockJumper_Check(void)
{
    // Debounce cooldown
    static uint32_t cooldown = 0;
    if (cooldown > 0) {
        cooldown--;
        return;
    }

    // Read the pin state
    bool old = SystemSettings.editable;
    LockJumper_ReadHardware();

    if (old != SystemSettings.editable) {
        // --- State changed ---
        cooldown = 5; // 0.5s if called every 100 ms

        jumper_changed();
    }
}

void LockJumper_ReadHardware(void)
{
    SystemSettings.editable = (bool) LL_GPIO_IsInputPinSet(lock_periph, lock_llpin);
}
