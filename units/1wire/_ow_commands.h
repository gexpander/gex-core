//
// Created by MightyPork on 2018/01/29.
//

#ifndef GEX_F072_OW_COMMANDS_H
#define GEX_F072_OW_COMMANDS_H

#ifndef OW_INTERNAL
#error bad include!
#endif

#define OW_ROM_SEARCH     0xF0
#define OW_ROM_READ       0x33
#define OW_ROM_MATCH      0x55
#define OW_ROM_SKIP       0xCC
#define OW_ROM_ALM_SEARCH 0xEC

#define OW_DS1820_CONVERT_T     0x44
#define OW_DS1820_WRITE_SCRATCH 0x4E
#define OW_DS1820_READ_SCRATCH  0xBE
#define OW_DS1820_COPY_SCRATCH  0x48
#define OW_DS1820_RECALL_E2     0xB8
#define OW_DS1820_READ_SUPPLY   0xB4

#endif //GEX_F072_OW_COMMANDS_H
