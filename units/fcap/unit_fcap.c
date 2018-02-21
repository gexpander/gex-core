//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "unit_fcap.h"

#define FCAP_INTERNAL
#include "_fcap_internal.h"

// ------------------------------------------------------------------------

enum FcapCmd_ {
    CMD_STOP = 0,
    CMD_INDIRECT_CONT_START = 1,
    CMD_INDIRECT_BURST_START = 2,
    CMD_DIRECT_CONT_START = 3,
    CMD_DIRECT_BURST_START = 4,
    CMD_FREERUNNING_START = 5,

    CMD_INDIRECT_CONT_READ = 10,
    CMD_DIRECT_CONT_READ = 11,
    CMD_FREERUNNING_READ = 12,
};

/** Handle a request message */
static error_t UFCAP_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    struct priv *priv = unit->data;
    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    uint16_t msec;

    const char* msg_denied_on_pin = "Not available on the selected pin!";

    switch (command) {
        case CMD_STOP:
            UFCAP_SwitchMode(unit, OPMODE_IDLE);
            return E_SUCCESS;

        case CMD_INDIRECT_CONT_START:
            if (priv->opmode == OPMODE_PWM_CONT) return E_SUCCESS; // no-op
            if (priv->opmode != OPMODE_IDLE) return E_BUSY;

            UFCAP_SwitchMode(unit, OPMODE_PWM_CONT);
            return E_SUCCESS;

        case CMD_INDIRECT_BURST_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;

            uint16_t count = pp_u16(pp);
            priv->pwm_burst.n_target = count;
            priv->request_id = frame_id;
            UFCAP_SwitchMode(unit, OPMODE_PWM_BURST);
            return E_SUCCESS;

        case CMD_INDIRECT_CONT_READ:
            if (priv->opmode != OPMODE_PWM_CONT) {
                return E_BAD_MODE;
            }
            if (priv->pwm_cont.last_period == 0) {
                return E_BUSY;
            }

            pb_u16(&pb, PLAT_AHB_MHZ);
            pb_u32(&pb, priv->pwm_cont.last_period);
            pb_u32(&pb, priv->pwm_cont.last_ontime);

            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        case CMD_DIRECT_CONT_START:
            if (!priv->a_direct) {
                // This works only if we use the ETR pin. TIM2 shares CH1 with ETR.
                // If CH2 is selected as input, ETR is not available.
                com_respond_str(MSG_ERROR, frame_id, msg_denied_on_pin);
                return E_FAILURE;
            }
            if (priv->opmode == OPMODE_COUNTER_CONT) return E_SUCCESS; // no-op
            if (priv->opmode != OPMODE_IDLE) return E_BUSY;

            msec = pp_u16(pp);
            priv->cnt_cont.msec = msec;

            UFCAP_SwitchMode(unit, OPMODE_COUNTER_CONT);
            return E_SUCCESS;

        case CMD_DIRECT_CONT_READ:
            if (!priv->a_direct) { // see above
                com_respond_str(MSG_ERROR, frame_id, msg_denied_on_pin);
                return E_FAILURE;
            }
            if (priv->opmode != OPMODE_COUNTER_CONT) return E_BAD_MODE;
            if (priv->cnt_cont.last_count == 0) return E_BUSY;

            pb_u32(&pb, priv->cnt_cont.last_count);
            pb_u16(&pb, priv->cnt_cont.msec);

            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        case CMD_DIRECT_BURST_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;

            msec = pp_u16(pp);
            priv->cnt_burst.msec = msec;
            priv->request_id = frame_id;
            UFCAP_SwitchMode(unit, OPMODE_COUNTER_BURST);
            return E_SUCCESS;

        case CMD_FREERUNNING_START:
            if (priv->opmode != OPMODE_IDLE) return E_BAD_MODE;
            UFCAP_SwitchMode(unit, OPMODE_COUNTER_FREERUNNING);
            return E_SUCCESS;

        case CMD_FREERUNNING_READ:
            if (priv->opmode != OPMODE_COUNTER_FREERUNNING) {
                return E_BAD_MODE;
            }

            pb_u32(&pb, UFCAP_GetFreeCounterValue(unit));
            com_respond_pb(frame_id, MSG_SUCCESS, &pb);
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

// ------------------------------------------------------------------------

/** Frequency capture */
const UnitDriver UNIT_FCAP = {
    .name = "FCAP",
    .description = "Frequency and pulse measurement",
    // Settings
    .preInit = UFCAP_preInit,
    .cfgLoadBinary = UFCAP_loadBinary,
    .cfgWriteBinary = UFCAP_writeBinary,
    .cfgLoadIni = UFCAP_loadIni,
    .cfgWriteIni = UFCAP_writeIni,
    // Init
    .init = UFCAP_init,
    .deInit = UFCAP_deInit,
    // Function
    .handleRequest = UFCAP_handleRequest,
};
