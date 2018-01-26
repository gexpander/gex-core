//
// Rename to TF_Config.h
//

#ifndef TF_CONFIG_H
#define TF_CONFIG_H

#include "platform.h"
#include <stdint.h>

#define TF_ID_BYTES     2
#define TF_LEN_BYTES    2
#define TF_TYPE_BYTES   1
#define TF_CKSUM_TYPE TF_CKSUM_XOR
#define TF_USE_SOF_BYTE 1
#define TF_SOF_BYTE     0x01
typedef uint16_t TF_TICKS;
typedef uint8_t TF_COUNT;
#define TF_USE_MUTEX  1

// buffer sizes and listener counts are in plat_compat.h

#define TF_Error(format, ...) dbg("[TF] " format, ##__VA_ARGS__)

#endif //TF_CONFIG_H
