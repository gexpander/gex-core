//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_LOCK_JUMPER_H
#define GEX_LOCK_JUMPER_H

#include "plat_compat.h"

/**
 * Init the lock jumper subsystem
 */
void LockJumper_Init(void);

/**
 * Check state of the lock jumper
 */
void LockJumper_Check(void);

#endif //GEX_LOCK_JUMPER_H
