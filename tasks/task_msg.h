//
// Created by MightyPork on 2017/12/22.
//

#ifndef GEX_F072_TASK_MSG_H
#define GEX_F072_TASK_MSG_H

extern osMessageQId queRxDataHandle;
extern osThreadId tskMsgHandle;
void TaskMessaging(const void * argument);

#endif //GEX_F072_TASK_MSG_H
