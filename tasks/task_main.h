//
// Created by MightyPork on 2017/11/09.
//

#ifndef GEX_TASK_MAIN_H
#define GEX_TASK_MAIN_H

#include "platform.h"

extern osThreadId tskMainHandle;
void TaskMain(const void *argument);

// Notify flags:
#define USBEVT_FLAG_EPx_IN(ep) (uint32_t)(1<<((ep)&0x7))
#define USBEVT_FLAG_EPx_OUT(ep) (uint32_t)(1<<(8 + ((ep)&0x7)))
#define USBEVT_FLAG_EP0_RX_RDY (1<<16)
#define USBEVT_FLAG_EP0_TX_SENT (1<<17)

#endif //GEX_TASK_MAIN_H
