//
// Created by MightyPork on 2017/12/04.
//

#include <task_sched.h>
#include "platform.h"
#include "stacksmon.h"

#if USE_STACK_MONITOR

struct stackhandle {
    const char *description;
    uint8_t *buffer;
    uint32_t len;
};

#define STACK_NUM 8
static uint32_t nextidx = 0;
static struct stackhandle stacks[STACK_NUM];

void stackmon_register(const char *description, void *buffer, uint32_t len)
{
    assert_param(nextidx < STACK_NUM);

    uint8_t *bv = buffer;

    // This is probably redundant, FreeRTOS does it too.
    for (uint32_t i = 0; i < len; i++) bv[i] = 0xA5;

    stacks[nextidx].description = description;
    stacks[nextidx].buffer = bv;
    stacks[nextidx].len = len;
    nextidx++;
}

void stackmon_dump(void)
{
    PUTS("\r\n\033[1m---- STACK USAGE REPORT ---\033[m\r\n");
    for (uint32_t i = 0; i < nextidx; i++) {
        PRINTF("\033[36m>> %s\033[m @ %p\r\n", stacks[i].description, (void *) stacks[i].buffer);
        struct stackhandle *stack = &stacks[i];
        uint32_t free = 0;
        if (stack->buffer[0] != 0xA5) {
            PUTS("   \033[31m!!! OVERRUN !!!\033[m\r\n");
        } else {
            for (uint32_t j = 0; j < stack->len; j++) {
                if (stack->buffer[j] == 0xA5) {
                    free++;
                } else {
                    break;
                }
            }
        }

        uint32_t words = ((stack->len-free)>>2)+1;
        if (words>stack->len>>2) words=stack->len>>2;

        PRINTF("   Used: \033[33m%"PRIu32" / %"PRIu32" bytes\033[m (\033[33m%"PRIu32" / %"PRIu32" words\033[m) ~ \033[33m%"PRIu32" %%\033[m\r\n",
               (stack->len-free),
               stack->len,
               words,
               stack->len>>2,
               (stack->len-free)*100/stack->len
        );
    }

    PUTS("\033[36m>> JOB QUEUE\033[m\r\n");
    PRINTF("   Used slots: \033[33m%"PRIu32"\033[m\r\n",
           jobQueHighWaterMark);

    PUTS("\033[36m>> MSG QUEUE\033[m\r\n");
    PRINTF("   Used slots: \033[33m%"PRIu32"\033[m\r\n",
           msgQueHighWaterMark);

    PRINTF("\033[1m---------------------------\033[m\r\n\r\n");
}


void stackmon_check_canaries(void)
{
    for (uint32_t i = 0; i < nextidx; i++) {
        struct stackhandle *stack = &stacks[i];
        if (stack->buffer[0] != 0xA5) {
            dbg("\r\n\033[31;1m!!!! STACK \"%s\" OVERRUN - CANARY IS DEAD !!!!\033[m", stack->description);
            stackmon_dump();
            trap("ABORT");
        }
    }
}

#endif
