//
// Created by MightyPork on 2018/07/07.
//

#ifndef GEX_CORE_PLAT_CONFIG_H
#define GEX_CORE_PLAT_CONFIG_H

#define VFS_DRIVE_NAME "GEX"

// -------- Priorities -------------
#define TSK_MAIN_PRIO osPriorityNormal
#define TSK_JOBS_PRIO osPriorityHigh
#define TSK_TIMERS_PRIO 4 // this must be in the 0-7 range

// -------- Static buffers ---------
// USB / VFS task stack size
#if DISABLE_MSC
#define TSK_STACK_MAIN      100 // without MSC the stack usage is significantly lower
#else
#define TSK_STACK_MAIN      160
#endif

// 180 is normally enough if not doing extensive debug logging
#define TSK_STACK_MSG       220 // TF message handler task stack size (all unit commands run on this thread)
#define TSK_STACK_IDLE    64 //configMINIMAL_STACK_SIZE
#define TSK_STACK_TIMERS  64 //configTIMER_TASK_STACK_DEPTH

#define PLAT_HEAP_SIZE 4096



#define BULK_READ_BUF_LEN 256   // Buffer for TF bulk reads
#define UNIT_TMP_LEN      256   // Buffer for internal unit operations

#define FLASH_SAVE_BUF_LEN  128 // Malloc'd buffer for saving to flash

#define MSG_QUE_SLOT_SIZE 64 // FIXME this should be possible to lower, but there's some bug with bulk transfer / INI parser
#define RX_QUE_CAPACITY    16 // TinyFrame rx queue size (64 bytes each)

#define TF_MAX_PAYLOAD_RX 512 // TF max Rx payload
#define TF_SENDBUF_LEN    512 // TF transmit buffer (can be less than a full frame)

#define TF_MAX_ID_LST   4 // Frame ID listener count
#define TF_MAX_TYPE_LST 6 // Frame Type listener count
#define TF_MAX_GEN_LST  1 // Generic listener count

#define USBD_MAX_STR_DESC_SIZ   64 // Descriptor conversion buffer (used for converting ASCII to UTF-16, must be 2x the size of the longest descriptor)
#define MSC_MEDIA_PACKET       512 // Mass storage sector size (packet)

#define INI_KEY_MAX    20 // Ini parser key buffer
#define INI_VALUE_MAX  30 // Ini parser value buffer

// -------- Stack buffers ----------
#define DBG_BUF_LEN      100 // Size of the snprintf buffer for debug messages
#define ERR_MSG_STR_LEN  64 // Error message buffer size
#define IWBUFFER_LEN     80 // Ini writer buffer for sprintf

// -------- Timeouts ------------
#define TF_PARSER_TIMEOUT_TICKS 100 // Timeout for receiving & parsing a frame
#define BULK_LST_TIMEOUT_MS     2000 // timeout for the bulk transaction to expire
#define MSG_QUE_POST_TIMEOUT    200 // Time to post to the messages / jobs queue


#endif //GEX_CORE_PLAT_CONFIG_H
