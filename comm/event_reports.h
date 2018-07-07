//
// Created by MightyPork on 2018/01/27.
//

#ifndef GEX_CORE_EVENT_REPORTS_H
#define GEX_CORE_EVENT_REPORTS_H

#ifndef GEX_MESSAGES_H
#error "Include messages.h instead!"
#endif

#include <TinyFrame.h>
#include "framework/unit.h"

/**
 * Event report object
 */
typedef struct event_report_ {
    Unit *unit;         //!< Reporting unit
    uint8_t type;       //!< Report type (if unit has multiple reports, otherwise 0)
    uint64_t timestamp; //!< Microsecond timestamp of the event, captured as close as possible to the IRQ
    uint16_t length;    //!< Payload length
    uint8_t *data;      //!< Data if using the EventReport_Send() function, otherwise NULL and data is sent using EventReport_Data()
    TF_ID sent_msg_id;
} EventReport;

bool EventReport_Send(EventReport *report);

bool EventReport_Start(EventReport *report);
void EventReport_Data(const uint8_t *buff, uint16_t len);
void EventReport_PB(PayloadBuilder *pb);
void EventReport_End(void);

#endif //GEX_CORE_EVENT_REPORTS_H
