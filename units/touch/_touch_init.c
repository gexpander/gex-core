//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TOUCH_INTERNAL
#include "_touch_internal.h"

/** Allocate data structure and set defaults */
error_t UTOUCH_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    priv->cfg.charge_time = 2;
    priv->cfg.drain_time = 2;
    priv->cfg.spread_deviation = 0;
    priv->cfg.ss_presc = 1;
    priv->cfg.pg_presc = 32;
    priv->cfg.sense_timeout = 7;
    memset(priv->cfg.group_scaps, 0, 8);
    memset(priv->cfg.group_channels, 0, 8);

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UTOUCH_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    //

    return E_SUCCESS;
}


/** Tear down the unit */
void UTOUCH_deInit(Unit *unit)
{
    struct priv *priv = unit->data;

    // de-init peripherals
    if (unit->status == E_SUCCESS ) {
        //
    }

    // Release all resources, deinit pins
    rsc_teardown(unit);

    // Free memory
    free_ck(unit->data);
}
