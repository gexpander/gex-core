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
#include "status_led.h"

static bool LockJumper_ReadPin(void);

static GPIO_TypeDef *lock_periph;
static uint32_t lock_llpin;

static bool old_state = false;
static uint32_t debo_ticks = 0;

// We use macros LOCK_JUMPER_PORT, LOCK_JUMPER_PIN from plat_compat.h

/** Init the jumper subsystem */
void LockJumper_Init(void)
{
    bool suc = true;

    // Resolve and claim resource
    Resource rsc = pin2resource(LOCK_JUMPER_PORT, LOCK_JUMPER_PIN, &suc);
    assert_param(suc);

    suc &= rsc_claim(&UNIT_SYSTEM, rsc);
    assert_param(suc);

    // Resolve pin
    lock_periph = port2periph(LOCK_JUMPER_PORT, &suc);
    lock_llpin = pin2ll(LOCK_JUMPER_PIN, &suc);
    assert_param(suc);

    // Configure for input
    LL_GPIO_SetPinMode(lock_periph, lock_llpin, LL_GPIO_MODE_INPUT);

    // pull-up/down in the opposite direction of the active state
    #if PLAT_LOCK_1CLOSED
        LL_GPIO_SetPinPull(lock_periph, lock_llpin, LL_GPIO_PULL_DOWN);
    #else
        LL_GPIO_SetPinPull(lock_periph, lock_llpin, LL_GPIO_PULL_UP);
    #endif

    SystemSettings.editable = (bool) LL_GPIO_IsInputPinSet(lock_periph, lock_llpin);
    dbg("Settings editable? %d", SystemSettings.editable);
}


/** Handle jumper state change */
static void jumper_changed(void)
{
    if (SystemSettings.editable) {
        // Unlock
        dbg("LOCK removed, enabling MSC!");
        Indicator_Effect(STATUS_DISK_ATTACHED);
    } else {
        // Lock
        dbg("LOCK replaced, disabling MSC!");
        Indicator_Effect(STATUS_DISK_REMOVED);

        if (SystemSettings.modified) {
            dbg("Saving settings to Flash...");
            settings_save();
        } else {
            dbg("No changes to save.");
        }
    }

    plat_usb_reconnect();
}


/** Periodic jumper check */
void LockJumper_Check(void)
{
    bool state = LockJumper_ReadPin();
    if (state != old_state) {
        old_state = state;
        debo_ticks = 3;
    }

    if (debo_ticks > 0) {
        if (--debo_ticks == 0) {
            dbg("Debo finished, state %d", (int)state);
            // we've reached a change
            #if PLAT_LOCK_BTN
                if (state == true) {
                    SystemSettings.editable = !SystemSettings.editable;
                    jumper_changed();
                }
            #else
                SystemSettings.editable = state;
                jumper_changed();
            #endif
        }
    }
}

/**
 * Read the jumper / button state.
 *
 * @return true if the jumper is closed, or button pressed
 */
static bool LockJumper_ReadPin(void)
{
    // lock jumper - start with MSC on if removed
    #if PLAT_LOCK_1CLOSED
        return (bool) LL_GPIO_IsInputPinSet(lock_periph, lock_llpin);
    #else
        return ! ((bool) LL_GPIO_IsInputPinSet(lock_periph, lock_llpin));
    #endif
}


void LockJumper_CheckInitialState(void)
{
    #if PLAT_LOCK_BTN
        // no static read - starts with MSC off
        SystemSettings.editable = false;
        old_state = false;
    #else
        old_state = SystemSettings.editable = ! LockJumper_ReadPin();
    #endif
}
