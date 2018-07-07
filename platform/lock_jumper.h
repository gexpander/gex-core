//
// Created by MightyPork on 2017/12/15.
//
// Handling the lock jumper or button (used to make the config filesystem visible to the PC OS)
//

#ifndef GEX_LOCK_JUMPER_H
#define GEX_LOCK_JUMPER_H

/**
 * Init the lock jumper subsystem
 */
void LockJumper_Init(void);

/**
 * Check state of the lock jumper
 */
void LockJumper_Check(void);

/**
 * Check hardware jumper and load it's value into the settings struct.
 * Does NOT trigger MSC changes or anything else.
 */
void LockJumper_CheckInitialState(void);

#endif //GEX_LOCK_JUMPER_H
