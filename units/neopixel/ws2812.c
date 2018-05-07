#include "platform.h"
#include "ws2812.h"

#define FREQ_STEP (PLAT_AHB_MHZ/20.0f)
#define NPX_DELAY_SHORT (uint32_t)(FREQ_STEP*1.5f)
#define NPX_DELAY_LONG  (uint32_t)(FREQ_STEP*3.5f)
#define NPX_DELAY_SHOW  (uint32_t)(FREQ_STEP*50)

static inline __attribute__((always_inline))
void ws2812_byte(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t b)
{
	for (register volatile uint8_t i = 0; i < 8; i++) {
        LL_GPIO_SetOutputPin(port, ll_pin);

		// duty cycle determines bit value
		if (b & 0x80) {
            __asm_loop(NPX_DELAY_LONG);
            LL_GPIO_ResetOutputPin(port, ll_pin);
            __asm_loop(NPX_DELAY_SHORT);
		} else {
            __asm_loop(NPX_DELAY_SHORT);
            LL_GPIO_ResetOutputPin(port, ll_pin);
            __asm_loop(NPX_DELAY_LONG);
		}

		b <<= 1; // shift to next bit
	}
}

/** Set many RGBs from packed stream */
void ws2812_load_raw(GPIO_TypeDef *port, uint32_t ll_pin, const uint8_t *rgbs, uint32_t count)
{
    vPortEnterCritical();
    uint8_t b, g, r;
	for (uint32_t i = 0; i < count; i++) {
		r = *rgbs++;
		g = *rgbs++;
		b = *rgbs++;
		ws2812_byte(port, ll_pin, g);
		ws2812_byte(port, ll_pin, r);
		ws2812_byte(port, ll_pin, b);
	}
    vPortExitCritical();
    __asm_loop(NPX_DELAY_SHOW);
}

/** Set many RGBs from uint32 stream */
void ws2812_load_sparse(GPIO_TypeDef *port, uint32_t ll_pin, const uint8_t *rgbs, uint32_t count,
                        bool order_bgr, bool zero_before)
{
    vPortEnterCritical();
    uint8_t b, g, r;
	for (uint32_t i = 0; i < count; i++) {
	    if (zero_before) rgbs++; // skip
        if (order_bgr) {
            b = *rgbs++;
            g = *rgbs++;
            r = *rgbs++;
        } else {
            r = *rgbs++;
            g = *rgbs++;
            b = *rgbs++;
        }
        if (!zero_before) rgbs++; // skip

		ws2812_byte(port, ll_pin, g);
		ws2812_byte(port, ll_pin, r);
		ws2812_byte(port, ll_pin, b);
	}
    vPortExitCritical();
    __asm_loop(NPX_DELAY_SHOW);
}

/** Set many RGBs */
void ws2812_clear(GPIO_TypeDef *port, uint32_t ll_pin, uint32_t count)
{
    vPortEnterCritical();
	for (uint32_t i = 0; i < count*3; i++) {
		ws2812_byte(port, ll_pin, 0);
	}
    vPortExitCritical();
    __asm_loop(NPX_DELAY_SHOW);
}
