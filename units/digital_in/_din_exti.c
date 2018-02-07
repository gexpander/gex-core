//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define DIN_INTERNAL

#include "_din_internal.h"

/**
 * Send a trigger event to master (called on the message queue thread).
 *
 * unit - unit
 * timestamp - timestamp
 * data1 - packed, triggering pin
 * data2 - snapshot
 */
static void DIn_SendTriggerReportToMaster(Job *job)
{
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    pb_u16(&pb, (uint16_t) job->data1); // packed, 1 on the triggering pin
    pb_u16(&pb, (uint16_t) job->data2); // packed, snapshot
    assert_param(pb.ok);

    EventReport event = {
        .unit = job->unit,
        .timestamp = job->timestamp,
        .data = pb.start,
        .length = (uint16_t) pb_length(&pb),
    };

    EventReport_Send(&event);
}

/**
 * EXTI callback for pin change interrupts
 *
 * @param arg - the unit is passed here
 */
void DIn_handleExti(void *arg)
{
    const uint64_t ts = PTIM_GetMicrotime();

    Unit *unit = arg;
    struct priv *priv = unit->data;
    const uint16_t snapshot = (uint16_t) priv->port->IDR;

    uint16_t trigger_map = 0;

    uint16_t mask = 1;
    const uint16_t armed_pins = priv->arm_single | priv->arm_auto;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (!LL_EXTI_ReadFlag_0_31(LL_EXTI_LINES[i])) continue;
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINES[i]);

        // Armed and ready
        if ((armed_pins & mask) && (priv->holdoff_countdowns[i] == 0)) {
            // Mark as captured
            trigger_map |= (1 << i);
            // Start hold-off (no-op if zero hold-off)
            priv->holdoff_countdowns[i] = priv->trig_holdoff;
        }
    }

    // Disarm all possibly used single triggers
    priv->arm_single &= ~trigger_map;

    if (trigger_map != 0) {
        Job j = {
            .unit = unit,
            .timestamp = ts,
            .data1 = pinmask_pack(trigger_map, priv->pins),
            .data2 = pinmask_pack(snapshot, priv->pins),
            .cb = DIn_SendTriggerReportToMaster
        };
        scheduleJob(&j);
    }
}
