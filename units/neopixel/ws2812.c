#include "platform.h"
#include "ws2812.h"

static //inline __attribute__((always_inline))
void ws2812_byte(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t b)
{
	for (register volatile uint8_t i = 0; i < 8; i++) {
        LL_GPIO_SetOutputPin(port, ll_pin);

		// duty cycle determines bit value
		if (b & 0x80) {
            __delay_cycles(20);
            //for(uint32_t _i = 0; _i < 10; _i++) asm volatile("nop");
            LL_GPIO_ResetOutputPin(port, ll_pin);
            //for(uint32_t _i = 0; _i < 10; _i++) asm volatile("nop");
            __delay_cycles(10);
		} else {
            __delay_cycles(10);
            //for(uint32_t _i = 0; _i < 5; _i++) asm volatile("nop");
            LL_GPIO_ResetOutputPin(port, ll_pin);
            __delay_cycles(20);
            //for(uint32_t _i = 0; _i < 10; _i++) asm volatile("nop");
		}

		b <<= 1; // shift to next bit
	}
}

/** Set many RGBs from packed stream */
void ws2812_load_raw(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t *rgbs, uint32_t count)
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
	// TODO: Delay 50 us
}

/** Set many RGBs from uint32 stream */
void ws2812_load_sparse(GPIO_TypeDef *port, uint32_t ll_pin, uint8_t *rgbs, uint32_t count, bool bigendian)
{
    vPortEnterCritical();
    uint8_t b, g, r;
	for (uint32_t i = 0; i < count; i++) {
        if (bigendian) {
            rgbs++; // skip
            b = *rgbs++;
            g = *rgbs++;
            r = *rgbs++;
        } else {
            r = *rgbs++;
            g = *rgbs++;
            b = *rgbs++;
            rgbs++; // skip
        }

		ws2812_byte(port, ll_pin, g);
		ws2812_byte(port, ll_pin, r);
		ws2812_byte(port, ll_pin, b);
	}
    vPortExitCritical();
	// TODO: Delay 50 us
}

/** Set many RGBs */
void ws2812_clear(GPIO_TypeDef *port, uint32_t ll_pin, uint32_t count)
{
    vPortEnterCritical();
	for (uint32_t i = 0; i < count*3; i++) {
		ws2812_byte(port, ll_pin, 0);
	}
    vPortExitCritical();
	// TODO: Delay 50 us
}
