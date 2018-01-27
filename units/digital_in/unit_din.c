//
// Created by MightyPork on 2017/11/25.
//

#include "unit_base.h"
#include "platform/irq_dispatcher.h"
#include "comm/messages.h"
#include "unit_din.h"
#include "tasks/task_msg.h"
#include "utils/avrlibc.h"

/** Private data structure */
struct priv {
    char port_name;
    uint16_t pins; // pin mask
    uint16_t pulldown; // pull-downs (default is pull-up)
    uint16_t pullup; // pull-ups
    uint16_t trig_rise; // pins generating events on rising edge
    uint16_t trig_fall; // pins generating events on falling edge
    uint16_t trig_holdoff; // ms
    uint16_t def_auto; // initial auto triggers

    uint16_t arm_auto;   // pins armed for auto reporting
    uint16_t arm_single; // pins armed for single event
    uint16_t holdoff_countdowns[16]; // countdowns to arm for each pin in the bit map
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
    if (version >= 2) {
        priv->def_auto = pp_u16(pp);
    }
}

/** Write to a binary buffer for storing in Flash */
static void DI_writeBinary(Unit *unit, PayloadBuilder *pb)
{
    struct priv *priv = unit->data;

    pb_u8(pb, 2); // version

    pb_char(pb, priv->port_name);
    pb_u16(pb, priv->pins);
    pb_u16(pb, priv->pulldown);
    pb_u16(pb, priv->pullup);
    pb_u16(pb, priv->trig_rise);
    pb_u16(pb, priv->trig_fall);
    pb_u16(pb, priv->trig_holdoff);
    pb_u16(pb, priv->def_auto);
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
    else if (streq(key, "auto-trigger")) {
        priv->def_auto = parse_pinmask(value, &suc);
    }
    else if (streq(key, "hold-off")) {
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

    iw_comment(iw, "Trigger pins activated by rising/falling edge");
    iw_entry(iw, "trig-rise", "%s", pinmask2str(priv->trig_rise, unit_tmp512));
    iw_entry(iw, "trig-fall", "%s", pinmask2str(priv->trig_fall, unit_tmp512));

    iw_comment(iw, "Trigger pins auto-armed by default");
    iw_entry(iw, "auto-trigger", "%s", pinmask2str(priv->def_auto, unit_tmp512));

    iw_comment(iw, "Triggers hold-off time (ms)");
    iw_entry(iw, "hold-off", "%d", (int)priv->trig_holdoff);

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
    priv->trig_holdoff = 100;
    priv->def_auto = 0x0000;

    return E_SUCCESS;
}

static void ID_SendTriggerReportToMaster(Job *job)
{
    Unit *unit = job->data1;

    PayloadBuilder pb = pb_start(unit_tmp512, UNIT_TMP_LEN, NULL);
    pb_u8(&pb, unit->callsign);
    pb_u8(&pb, 0x00); // report type "Trigger"
    {
        pb_u16(&pb, (uint16_t) job->d32_2); // packed, 1 on the triggering pin
        pb_u16(&pb, (uint16_t) job->d32_3); // packed, snapshot
        // the snapshot can be used to capture the other input pins
    }
    assert_param(pb.ok);
    com_send_pb(MSG_UNIT_REPORT, &pb);
}

static void DI_handleExti(void *arg)
{
    Unit *unit = arg;
    struct priv *priv = unit->data;
    const uint16_t snapshot = (uint16_t) priv->port->IDR;

    uint16_t trigger_map = 0;

    uint16_t mask = 1;
    const uint16_t armed_pins = priv->arm_single|priv->arm_auto;
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
            .data1 = unit,
            .d32_2 = pinmask_pack(trigger_map, priv->pins),
            .d32_3 = pinmask_pack(snapshot, priv->pins),
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
    priv->trig_rise &= priv->pins;
    priv->trig_fall &= priv->pins;
    priv->def_auto &= (priv->trig_rise|priv->trig_fall);

    // copy auto-arm defaults to the auto-arm register (the register may be manipulated by commands)
    priv->arm_auto = priv->def_auto;
    priv->arm_single = 0;

    // clear countdowns
    memset(priv->holdoff_countdowns, 0, sizeof(priv->holdoff_countdowns));

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

    // request ticks if we have triggers and any hold-offs configured
    if ((priv->trig_rise|priv->trig_fall) && priv->trig_holdoff > 0) {
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

/** Arm pins */
error_t UU_DI_Arm(Unit *unit, uint16_t arm_single_packed, uint16_t arm_auto_packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;

    uint16_t arm_single = pinmask_spread(arm_single_packed, priv->pins);
    uint16_t arm_auto = pinmask_spread(arm_auto_packed, priv->pins);

    // abort if user tries to arm pin that doesn't have a trigger configured
    if (0 != ((arm_single|arm_auto) & ~(priv->trig_fall|priv->trig_rise))) {
        return E_BAD_VALUE;
    }

    // arm and reset hold-offs
    // we use critical section to avoid irq between the two steps
    vPortEnterCritical();
    {
        priv->arm_auto |= arm_single;
        priv->arm_single |= arm_auto;
        const uint16_t combined = arm_single | arm_auto;
        for (int i = 0; i < 16; i++) {
            if (combined & (1 << i)) {
                priv->holdoff_countdowns[i] = 0;
            }
        }
    }
    vPortExitCritical();

    return E_SUCCESS;
}

/** DisArm pins */
error_t UU_DI_DisArm(Unit *unit, uint16_t disarm_packed)
{
    CHECK_TYPE(unit, &UNIT_DIN);
    struct priv *priv = unit->data;

    uint16_t disarm = pinmask_spread(disarm_packed, priv->pins);

    // abort if user tries to disarm pin that doesn't have a trigger configured
    if (0 != ((disarm) & ~(priv->trig_fall|priv->trig_rise))) {
        return E_BAD_VALUE;
    }

    priv->arm_auto &= ~disarm;
    priv->arm_single &= ~disarm;

    return E_SUCCESS;
}

enum PinCmd_ {
    CMD_READ = 0,
    CMD_ARM_SINGLE = 1,
    CMD_ARM_AUTO = 2,
    CMD_DISARM = 3,
};

/** Handle a request message */
static error_t DI_handleRequest(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp)
{
    uint16_t pins = 0;

    switch (command) {
        case CMD_READ:;
            TRY(UU_DI_Read(unit, &pins));

            PayloadBuilder pb = pb_start((uint8_t*)unit_tmp512, UNIT_TMP_LEN, NULL);
            pb_u16(&pb, pins); // packed input pins
            com_respond_buf(frame_id, MSG_SUCCESS, (uint8_t *) unit_tmp512, pb_length(&pb));
            return E_SUCCESS;

        case CMD_ARM_SINGLE:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_Arm(unit, pins, 0));
            return E_SUCCESS;

        case CMD_ARM_AUTO:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_Arm(unit, 0, pins));
            return E_SUCCESS;

        case CMD_DISARM:;
            pins = pp_u16(pp);
            if (!pp->ok) return E_MALFORMED_COMMAND;

            TRY(UU_DI_DisArm(unit, pins));
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
    .description = "Digital input with triggers",
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
