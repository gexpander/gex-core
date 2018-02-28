//
// Created by MightyPork on 2018/02/27.
//

#ifndef GEX_F072_WATCHDOG_H
#define GEX_F072_WATCHDOG_H

/**
 * Initialize the application watchdog
 */
void wd_init(void);

/**
 * Suspend watchdog restarts until resumed
 * (used in other tasks to prevent the main task clearing the wd if the other task is locked up)
 *
 * The suspend/resume calls can be stacked.
 */
void wd_suspend(void);

/**
 * Resume restarts
 */
void wd_resume(void);

/**
 * Restart the wd. If restarts are suspended, postpone the restart until resumed
 * and then restart immediately.
 */
void wd_restart(void);

#endif //GEX_F072_WATCHDOG_H
