//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "utils/avrlibc.h"
#include "platform/irq_dispatcher.h"
#include "comm/messages.h"
#include "unit_din.h"
#include "tasks/task_msg.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t pulldown; // pull-downs (default is pull-up)
    uint16_t pullup; // pull-ups
    uint16_t trig_rise;
    uint16_t trig_fall;
    uint16_t trig_holdoff; // ms

    uint16_t holdoff_countdowns[16];
    GPIO_TypeDef *port;
};

// ------------------------------------------------------------------------

/** Load from a binary buffer stored in Flash */
static void DI_loadBinary(Unit *unit, PayloadParser *pp)
{
    struct priv *priv = unit->data;

    uint8_t version = pp_u8(pp);
    (void)version;

    priv->port_name = pp_char(pp);
    priv->pins = pp_u16(pp);
    priv->pulldown = pp_u16(pp);
    priv->pullup = pp_u16(pp);

    if (version >= 1) {
        priv->trig_rise = pp_u16(pp);
        priv->trig_fall = pp_u16(pp);
        priv->trig_holdoff = pp_u16(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
static void DI_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 1); // version

    pb_char(pb, priv->port_name);
    pb_u16(pb, priv->pins);
    pb_u16(pb, priv->pulldown);
    pb_u16(pb, priv->pullup);
    pb_u16(pb, priv->trig_rise);
    pb_u16(pb, priv->trig_fall);
    pb_u16(pb, priv->trig_holdoff);
}

// ------------------------------------------------------------------------

/** Parse a key-value pair from the INI file */
static error_t DI_loadIni(Unit *unit, const char *key, const char *value)
{
    bool suc = true;
    struct priv *priv = unit->data;

    if (streq(key, "port")) {
        suc = parse_port_name(value, &priv->port_name);
    }
    else if (streq(key, "pins")) {
        priv->pins = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-up")) {
        priv->pullup = parse_pinmask(value, &suc);
    }
    else if (streq(key, "pull-down")) {
        priv->pulldown = parse_pinmask(value, &suc);
    }
    else if (streq(key, "trig-rise")) {
        priv->trig_rise = parse_pinmask(value, &suc);
    }
    else if (streq(key, "trig-fall")) {
        priv->trig_fall = parse_pinmask(value, &suc);
    }
    else if (streq(key, "trig-hold-off")) {
        priv->trig_holdoff = (uint16_t) avr_atoi(value);
    }
    else {
        return E_BAD_KEY;
    }

    if (!suc) return E_BAD_VALUE;
    return E_SUCCESS;
}

/** Generate INI file section for the unit */
static void DI_writeIni(Unit *unit, IniWriter *iw)
{
    struct priv *priv = unit->data;

    iw_comment(iw, "Port name");
    iw_entry(iw, "port", "%c", priv->port_name);

    iw_comment(iw, "Pins (comma separated, supports ranges)");
    iw_entry(iw, "pins", "%s", pinmask2str(priv->pins, unit_tmp512));

    iw_comment(iw, "Pins with pull-up");
    iw_entry(iw, "pull-up", "%s", pinmask2str(priv->pullup, unit_tmp512));

    iw_comment(iw, "Pins with pull-down");
    iw_entry(iw, "pull-down", "%s", pinmask2str(priv->pulldown, unit_tmp512));

    iw_comment(iw, "Trigger pins activated by rising edge");
    iw_entry(iw, "trig-rise", "%s", pinmask2str(priv->trig_rise, unit_tmp512));

    iw_comment(iw, "Trigger pins activated by falling edge");
    iw_entry(iw, "trig-fall", "%s", pinmask2str(priv->trig_fall, unit_tmp512));

    iw_comment(iw, "Trigger hold-off time (ms)");
    iw_entry(iw, "trig-hold-off", "%d", (int)priv->trig_holdoff);

#if PLAT_NO_FLOATING_INPUTS
    iw_comment(iw, "NOTE: Pins use pull-up by default.\r\n");
#endif
}

// ------------------------------------------------------------------------

/** Allocate data structure and set defaults */
static error_t DI_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    // some defaults
    priv->port_name = 'A';
    priv->pins = 0x0001;
    priv->pulldown = 0x0000;
    priv->pullup = 0x0000;

    priv->trig_rise = 0x0000;
    priv->trig_fall = 0x0000;
    priv->trig_holdoff = 20;

    return E_SUCCESS;
}

static void ID_SendTriggerReportToMaster(Job *job)
{
    Unit *unit = job->data1;

    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    pb_u8(&pb, unit->callsign);
    pb_u8(&pb, 0x00); // report type "Trigger"
    pb_u16(&pb, (uint16_t) job->d32_2); // packed, 1 on the triggering pin
    pb_u16(&pb, (uint16_t) job->d32_3); //
    assert_param(pb.ok);
    com_send_pb(MSG_UNIT_REPORT, &pb);
}

static void DI_handleExti(void *arg)
{
    Unit *unit = arg;
    struct priv *priv = unit->data;
    uint16_t snapshot = (uint16_t) priv->port->IDR;

    uint16_t trigger_map = 0;

    uint16_t mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if ((priv->trig_rise|priv->trig_fall) & mask) {
            if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINES[i])) {
                if (priv->holdoff_countdowns[i] == 0) {
                    trigger_map |= 1<<i;
                    priv->holdoff_countdowns[i] = priv->trig_holdoff;
                }

                LL_EXTI_ClearFlag_0_31(LL_EXTI_LINES[i]);
            }
        }
    }

    // FIXME the job is not correctly handled, somehow. queue fills but is not collected
    if (trigger_map != 0) {
        Job j = {
            .data1 = unit,
            .d32_2 = pinmask_pack(trigger_map, priv->pins),
            .d32_3 = snapshot,
            .cb = ID_SendTriggerReportToMaster
        };
        scheduleJob(&j);
    }
}

/** Finalize unit set-up */
static error_t DI_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    priv->pulldown &= priv->pins;
    priv->pullup &= priv->pins;

    // --- Parse config ---
    priv->port = hw_port2periph(priv->port_name, &suc);
    if (!suc) return E_BAD_CONFIG;

    // Claim all needed pins
    TRY(rsc_claim_gpios(unit, priv->port_name, priv->pins));

    uint16_t mask = 1;
    for (int i = 0; i < 16; i++, mask <<= 1) {
        if (priv->pins & mask) {
            uint32_t ll_pin = hw_pin2ll((uint8_t) i, &suc);

            // --- Init hardware ---
            LL_GPIO_SetPinMode(priv->port, ll_pin, LL_GPIO_MODE_INPUT);

            uint32_t pull = 0;

            #if PLAT_NO_FLOATING_INPUTS
                pull = LL_GPIO_PULL_UP;
            #else
                pull = LL_GPIO_PULL_NO;
            #endif

            if (priv->pulldown & mask) pull = LL_GPIO_PULL_DOWN;
            if (priv->pullup & mask) pull = LL_GPIO_PULL_UP;
            LL_GPIO_SetPinPull(priv->port, ll_pin, pull);

            if ((priv->trig_rise|priv->trig_fall) & mask) {
                LL_EXTI_EnableIT_0_31(LL_EXTI_LINES[i]);

                if (priv->trig_rise & mask) {
                    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINES[i]);
                }
                if (priv->trig_fall & mask) {
                    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINES[i]);
                }

                LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTS[priv->port_name-'A'], LL_SYSCFG_EXTI_LINES[i]);

                irqd_attach(EXTIS[i], DI_handleExti, unit);
            }
        }
    }

    // request ticks if we have triggers
    if (priv->trig_rise|priv->trig_fall) {
        unit->tick_interval = 1;
    }

    return E_SUCCESS;
}


/** Tear down the unit */
static void DI_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // pins are de-inited during teardown

    if (unit->status == E_SUCCESS) {
        // Detach EXTI handlers and disable interrupts
        if (priv->trig_rise | priv->trig_fall) {
            uint16_t mask = 1;
            for (int i = 0; i < 16; i++, mask <<= 1) {
                if ((priv->trig_rise | priv->trig_fall) & mask) {
                    LL_EXTI_DisableIT_0_31(LL_EXTI_LINES[i]);
                    irqd_detach(EXTIS[i], DI_handleExti);
                }
            }
        }
    }

    // Release all resources
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}

// ------------------------------------------------------------------------

/** Read request */
error_t UU_DI_Read(Unit *unit, uint16_t *packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;
    *packed = pinmask_pack((uint16_t) priv->port->IDR, priv->pins);
    return E_SUCCESS;
}

enum PinCmd_ {
    CMD_READ = 0,
};

/** Handle a request message */
static error_t DI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t packed = 0;

    switch (command) {
        case CMD_READ:;
            TRY(UU_DI_Read(unit, &packed));

            PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u16(&pb, packed);
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, pb_length(&pb));
            return E_SUCCESS;

        default:
            return E_UNKNOWN_COMMAND;
    }
}

/**
 * Decrement all the hold-off timers on tick
 *
 * @param unit
 */
static void DI_updateTick(Unit *unit)
{
    struct priv *priv = unit->data;

    for (int i = 0; i < 16; i++) {
        if (priv->holdoff_countdowns[i] > 0) {
            priv->holdoff_countdowns[i]--;
        }
    }
}

// ------------------------------------------------------------------------

/** Unit template */
const UnitDriver UNIT_DIN = {
    .name = "DI",
    .description = "Digital input",
    // Settings
    .preInit = DI_preInit,
    .cfgLoadBinary = DI_loadBinary,
    .cfgWriteBinary = DI_writeBinary,
    .cfgLoadIni = DI_loadIni,
    .cfgWriteIni = DI_writeIni,
    // Init
    .init = DI_init,
    .deInit = DI_deInit,
    // Function
    .handleRequest = DI_handleRequest,
    .updateTick = DI_updateTick,
};
