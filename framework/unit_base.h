//
// Created by MightyPork on 2017/12/09.
//
// This is a common include file used in unit drivers to avoid repeating all the
// needed includes everywhere.
//

#include "platform.h"
#include "unit.h"
#include "hw_utils.h"
#include "cfg_utils.h"
#include "resources.h"
#include "utils/str_utils.h"
#include "utils/malloc_safe.h"
#include "payload_builder.h"
#include "payload_parser.h"
#include "utils/avrlibc.h"
#include "tasks/task_msg.h"
#include "platform/timebase.h"
#include "platform/irq_dispatcher.h"
#include "comm/messages.h"
