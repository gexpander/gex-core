//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "framework/resources.h"
#include "status_led.h"
#include "pin_utils.h"

static GPIO_TypeDef *led_periph;
static uint32_t led_llpin;

static uint32_t active_effect = STATUS_NONE;
static uint32_t effect_time = 0;

// counter of idle ticks since last indicator
// used to allow or disallow heartbeat blink (to avoid interference)
static uint32_t indicator_idle_ms = 0;
#define IDLE_FOR_HEARTBEAT_MS 3000
#define HB_MAX_SAFE_IVAL 500

static uint32_t hb_elapsed = 0;

/** Early init */
void Indicator_PreInit(void)
{
    bool suc = true;
    // Resolve pin
    led_periph = port2periph(STATUS_LED_PORT, &suc);
    led_llpin = pin2ll(STATUS_LED_PIN, &suc);

    // Configure for output
    LL_GPIO_SetPinMode(led_periph, led_llpin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(led_periph, led_llpin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(led_periph, led_llpin, LL_GPIO_SPEED_FREQ_LOW);

    assert_param(suc);
}

static inline void led_on(void)
{
    LL_GPIO_SetOutputPin(led_periph, led_llpin);
}

static inline void led_off(void)
{
    LL_GPIO_ResetOutputPin(led_periph, led_llpin);
}

/** Set up the LED */
void Indicator_Init(void)
{
    bool suc = true;

    // Resolve and claim resource
    Resource rsc = pin2resource(STATUS_LED_PORT, STATUS_LED_PIN, &suc);
    assert_param(suc);

    suc &= rsc_claim(&UNIT_SYSTEM, rsc);
    assert_param(suc);
}

/** Set indicator ON */
void Indicator_Effect(enum GEX_StatusIndicator indicator)
{
    if (indicator == STATUS_FAULT) {
        // Persistent light - start immediately
        led_on();
    }

    active_effect = indicator;
    effect_time = 0;
}

void Indicator_Off(enum GEX_StatusIndicator indicator)
{
    if (active_effect == indicator) {
        led_off();
        active_effect = STATUS_NONE;
        indicator_idle_ms = 0;
    }
}

/** Millisecond tick */
void Indicator_Tick(void)
{
    if (active_effect == STATUS_NONE) {
        indicator_idle_ms++;

        if (hb_elapsed < HB_MAX_SAFE_IVAL && indicator_idle_ms > IDLE_FOR_HEARTBEAT_MS &&
            (indicator_idle_ms % 10 == 0)) {
            Indicator_Effect(STATUS_HEARTBEAT);
        }
    }

    if (active_effect != STATUS_NONE) {
        indicator_idle_ms = 0;

        if (active_effect == STATUS_HEARTBEAT) {
            if (effect_time == 0) led_on();
            else if (effect_time == 50) {
                led_off();
                active_effect = STATUS_NONE;
            }
        }
        else if (active_effect == STATUS_DISK_ATTACHED) {
            if (effect_time == 0) led_on();
            else if (effect_time == 100) led_off();
            else if (effect_time == 200) led_on();
            else if (effect_time == 700) {
                led_off();
                active_effect = STATUS_NONE;
            }
        }
        else if (active_effect == STATUS_DISK_REMOVED) {
            if (effect_time == 0) led_on();
            else if (effect_time == 500) led_off();
            else if (effect_time == 600) led_on();
            else if (effect_time == 700) {
                led_off();
                active_effect = STATUS_NONE;
            }
        }
        else if (active_effect == STATUS_DISK_BUSY) {
            if (effect_time == 600) {
                led_off();
                active_effect = STATUS_NONE;
            }
            else if (effect_time % 200 == 0) led_on();
            else if (effect_time % 200 == 100) led_off();
        }
        else if (active_effect == STATUS_WELCOME) {
            if (effect_time == 0) led_on();
            else if (effect_time == 200) led_off();
            else if (effect_time == 300) led_on();
            else if (effect_time == 500) {
                led_off();
                active_effect = STATUS_NONE;
            }
        }

        effect_time++;
    }
}

/** Heartbeat callback from the main thread */
void Indicator_Heartbeat(void)
{
    hb_elapsed = 0;
    // this is called every ~ 100 ms from the main loop
}
