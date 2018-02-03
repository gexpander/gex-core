//
// Created by MightyPork on 2018/02/03.
//

#ifndef GEX_F072_DIN_EXTI_H
#define GEX_F072_DIN_EXTI_H

#ifndef DIN_INTERNAL
#error bad include!
#endif

#include "unit_base.h"

/**
 * EXTI callback for pin change interrupts
 *
 * @param arg - the unit is passed here
 */
void DI_handleExti(void *arg);

#endif //GEX_F072_DIN_EXTI_H
