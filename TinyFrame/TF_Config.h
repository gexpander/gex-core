//
// Rename to TF_Config.h
//

#ifndef TF_CONFIG_H
#define TF_CONFIG_H

#include "platform.h"
#include <stdint.h>
//#include <esp8266.h> // when using with esphttpd

//----------------------------- FRAME FORMAT ---------------------------------
// The format can be adjusted to fit your particular application needs

// If the connection is reliable, you can disable the SOF byte and checksums.
// That can save up to 9 bytes of overhead.

// ,-----+----+-----+------+------------+- - - -+------------,
// | SOF | ID | LEN | TYPE | HEAD_CKSUM | DATA  | PLD_CKSUM  |
// | 1   | ?  | ?   | ?    | ?          | ...   | ?          | <- size (bytes)
// '-----+----+-----+------+------------+- - - -+------------'

// !!! BOTH SIDES MUST USE THE SAME SETTINGS !!!

// Adjust sizes as desired (1,2,4)
#define TF_ID_BYTES     2
#define TF_LEN_BYTES    2
#define TF_TYPE_BYTES   1

// Checksum type
//#define TF_CKSUM_TYPE TF_CKSUM_NONE
#define TF_CKSUM_TYPE TF_CKSUM_XOR
//#define TF_CKSUM_TYPE TF_CKSUM_CRC16
//#define TF_CKSUM_TYPE TF_CKSUM_CRC32

// Use a SOF byte to mark the start of a frame
#define TF_USE_SOF_BYTE 1
// Value of the SOF byte (if TF_USE_SOF_BYTE == 1)
#define TF_SOF_BYTE     0x01

//----------------------- PLATFORM COMPATIBILITY ----------------------------

// used for timeout tick counters - should be large enough for all used timeouts
typedef uint16_t TF_TICKS;

// used in loops iterating over listeners
typedef uint8_t TF_COUNT;

//----------------------------- PARAMETERS ----------------------------------

// buffers, counts and timeout are defined in plat_compat.h

#endif //TF_CONFIG_H
