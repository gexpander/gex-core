//
// Created by MightyPork on 2017/12/15.
//

#include "platform.h"
#include "framework/resources.h"
#include "status_led.h"
#include "pin_utils.h"

static uint32_t indicators[_INDICATOR_COUNT];

static GPIO_TypeDef *led_periph;
static uint32_t led_llpin;

/** Early init */
void StatusLed_PreInit(void)
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

/** Set up the LED */
void StatusLed_Init(void)
{
    bool suc = true;

    // Resolve and claim resource
    Resource rsc = pin2resource(STATUS_LED_PORT, STATUS_LED_PIN, &suc);
    assert_param(suc);

    suc &= rsc_claim(&UNIT_SYSTEM, rsc);
    assert_param(suc);
}

/** Set indicator ON */
void StatusLed_On(enum GEX_StatusIndicator indicator)
{
    indicators[indicator] = osWaitForever;

    if (indicator == STATUS_FAULT) {
        // Persistent light
        LL_GPIO_SetOutputPin(led_periph, led_llpin);
    }
}

/** Set indicator OFF */
void StatusLed_Off(enum GEX_StatusIndicator indicator)
{
    indicators[indicator] = 0;
    // TODO some effect
}

/** Set or reset a indicator */
void StatusLed_Set(enum GEX_StatusIndicator indicator, bool set)
{
    if (set) {
        StatusLed_On(indicator);
    } else {
        StatusLed_Off(indicator);
    }
}

/** Turn indicator ON for a given interval */
void StatusLed_Flash(enum GEX_StatusIndicator indicator, uint32_t ms)
{
    indicators[indicator] = ms;
    // TODO
}

/** Millisecond tick */
void StatusLed_Tick(void)
{
    for (uint32_t i = 0; i < _INDICATOR_COUNT; i++) {
        if (indicators[i] != osWaitForever && indicators[i] != 0) {
            if (--indicators[i]) {
                StatusLed_Off((enum GEX_StatusIndicator) i);
            }
        }
    }
}

/** Heartbeat callback from the main thread */
void StatusLed_Heartbeat(void)
{
    // TODO fixme
    LL_GPIO_TogglePin(led_periph, led_llpin);
}
