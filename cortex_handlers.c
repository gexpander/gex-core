/* Includes ------------------------------------------------------------------*/

#include "platform.h"
#include <TinyFrame.h>
#include "platform/debug_uart.h"
#include "platform/status_led.h"
#include "utils/stacksmon.h"

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

#define tFAULT "\r\n\033[31mSYSTEM FAULT:\033[m"

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
    called if a stack overflow is detected. */
    PRINTF(tFAULT" RTOS stack overflow! tsk: %s\r\n", (char *) pcTaskName);
    StatusLed_On(STATUS_FAULT);

    stackmon_dump();
    while (1);
}

#if VERBOSE_HARDFAULT && (__CORTEX_M >= 3)
void prvGetRegistersFromStack( uint32_t *origStack, uint32_t lr_value)
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
    volatile uint32_t stacked_r0;
    volatile uint32_t stacked_r1;
    volatile uint32_t stacked_r2;
    volatile uint32_t stacked_r3;
    volatile uint32_t stacked_r12;
    volatile uint32_t stacked_lr; /* Link register. */
    volatile uint32_t stacked_pc; /* Program counter. */
    volatile uint32_t stacked_psr;/* Program status register. */

    uint32_t cfsr, hfsr, dfsr;
    uint32_t bus_fault_address;
    uint32_t memmanage_fault_address;

    bus_fault_address       = SCB->BFAR;
    memmanage_fault_address = SCB->MMFAR;
    cfsr = SCB->CFSR;
    hfsr = SCB->HFSR;
    dfsr = SCB->DFSR;

    stacked_r0 = origStack[0];
    stacked_r1 = origStack[1];
    stacked_r2 = origStack[2];
    stacked_r3 = origStack[3];
    stacked_r12 = origStack[4];
    stacked_lr = origStack[5];
    stacked_pc = origStack[6];
    stacked_psr = origStack[7];

#define BS(reg, pos, str) (((reg)&(1<<(pos)))?(str" "):"")
#define REDPTR(val)  (((val)&0xFF000000) != 0x08000000?"\033[31m":"\033[32m")

    PRINTF(tFAULT" HARD FAULT\r\n\r\n");
    PRINTF("- Stack frame:\r\n");
    PRINTF(" R0  = \033[35m%"PRIX32"h\033[m\r\n", stacked_r0);
    PRINTF(" R1  = \033[35m%"PRIX32"h\033[m\r\n", stacked_r1);
    PRINTF(" R2  = \033[35m%"PRIX32"h\033[m\r\n", stacked_r2);
    PRINTF(" R3  = \033[35m%"PRIX32"h\033[m\r\n", stacked_r3);
    PRINTF(" R12 = \033[35m%"PRIX32"h\033[m\r\n", stacked_r12);
    PRINTF(" LR  = %s0x%08"PRIX32"\033[m\r\n", REDPTR(stacked_lr), stacked_lr);
    PRINTF(" PC  = %s0x%08"PRIX32"\033[m\r\n", REDPTR(stacked_pc), stacked_pc);
    PRINTF(" PSR = \033[36m0x%08"PRIX32"\033[m", stacked_psr);
    uint32_t exc = stacked_psr & 0x3F;
    PRINTF(" [ %s%s%s%s%s ]\r\n",
            BS(stacked_psr, 31, "N"),
            BS(stacked_psr, 30, "Z"),
            BS(stacked_psr, 29, "C"),
            BS(stacked_psr, 28, "V"),
        //BS(stacked_psr, 24, "T"), - thumb, always ON

            (exc==0)?"Thread":
            (exc==2)?"NMI":
            (exc==3)?"HardFault":
            (exc==11)?"SVCall":
            (exc==14)?"PendSV":
            (exc==15)?"SysTick":
            (exc>=16)?"IRQ":"Unknown"
    );

    PRINTF("\r\n- FSR/FAR:\r\n");
    PRINTF(" CFSR = \033[36m0x%08"PRIX32"\033[m\r\n", cfsr);
    PRINTF("      UsageFault: \033[31;1m%s%s%s%s%s%s%s\033[m\r\n"
                "      BusFault:   \033[31;1m%s%s%s%s%s%s%s%s\033[m\r\n"
                "      MemFault:   \033[31;1m%s%s%s%s%s%s%s\033[m\r\n",
            BS(cfsr, 0, "IAccViol"),
            BS(cfsr, 1, "DAccViol"),
            BS(cfsr, 3, "MUnstkErr"),
            BS(cfsr, 4, "MStkErr"),
            BS(cfsr, 5, "MLSPErr(FPU)"),
            BS(cfsr, 7, "MMArValid"),
            ((cfsr&0xFF)?"":"\033[m- "),

            BS(cfsr, 8, "IBusErr"),
            BS(cfsr, 9, "PreciseErr"),
            BS(cfsr, 10, "ImpreciseErr"),
            BS(cfsr, 11, "UnstkErr"),
            BS(cfsr, 12, "StkErr"),
            BS(cfsr, 13, "LSPErr"),
            BS(cfsr, 15, "BFArValid"),
            ((cfsr&0xFF00)?"":"\033[m- "),

            BS(cfsr, 16, "UndefInstr"),
            BS(cfsr, 17, "InvState"),
            BS(cfsr, 18, "InvPC"),
            BS(cfsr, 19, "NoCP"),
            BS(cfsr, 24, "Unaligned"),
            BS(cfsr, 25, "Div0"),
            ((cfsr&0xFFFF0000)?"":"\033[m- ")
    );

    PRINTF(" HFSR = \033[36m0x%08"PRIX32"\033[m", hfsr);
    PRINTF(" [ %s%s%s]\r\n",
            BS(hfsr, 31, "DebugEvt"),
            BS(hfsr, 30, "Forced"),
            BS(hfsr, 1, "VectTbl")
    );

    PRINTF(" DFSR = \033[36m0x%08"PRIX32"\033[m", dfsr);
    PRINTF(" [ %s%s%s%s%s]\r\n",
            BS(dfsr, 0, "Halted"),
            BS(dfsr, 1, "Bkpt"),
            BS(dfsr, 2, "DWtTrap"),
            BS(dfsr, 3, "VCatch"),
            BS(dfsr, 4, "External")
    );

    if (cfsr & 0x0080) PRINTF(" MMFAR = \033[33m0x%08"PRIX32"\033[m\r\n", memmanage_fault_address);
    if (cfsr & 0x8000) PRINTF(" BFAR = \033[33m0x%08"PRIX32"\033[m\r\n", bus_fault_address);
    PRINTF("\r\n- Misc\r\n");
    PRINTF(" LR/EXC_RETURN= %s0x%08"PRIX32"\033[m\n", REDPTR(lr_value), lr_value);

    StatusLed_On(STATUS_FAULT);
    while (1);
}
#endif

/**
* @brief This function handles Hard fault interrupt.
*/
void  __attribute__((naked)) HardFault_Handler(void)
{
#if VERBOSE_HARDFAULT && (__CORTEX_M >= 3)
    __asm volatile
    (
    " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " mov r2, lr                                                \n"
        " ldr r3, handler2_address_const                            \n"
        " bx r3                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
#endif

    PRINTF(tFAULT" HARD FAULT\r\n\r\n");
    StatusLed_On(STATUS_FAULT);
    while (1);
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
