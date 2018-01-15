#include "platform.h"
#include "platform/status_led.h"

void _abort_errlight(void)
{
    Indicator_Effect(STATUS_FAULT);
}

void __attribute__((noreturn)) _abort_do(void)
{
    vPortEnterCritical();
    while(1);
}

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   */
void __attribute__((noreturn)) _assert_failed(const char *file, uint32_t line)
{
    _abort_msg(file, line, "ASSERT FAILED");
}
