//
// Created by MightyPork on 2018/02/03.
//

#include "platform.h"
#include "unit_base.h"

#define TPL_INTERNAL
#include "_tpl_internal.h"

/** Allocate data structure and set defaults */
error_t UTPL_preInit(Unit *unit)
{
    struct priv *priv = unit->data = calloc_ck(1, sizeof(struct priv));
    if (priv == NULL) return E_OUT_OF_MEM;

    //

    return E_SUCCESS;
}

/** Finalize unit set-up */
error_t UTPL_init(Unit *unit)
{
    bool suc = true;
    struct priv *priv = unit->data;

    //

    return E_SUCCESS;
}


/** Tear down the unit */
void UTPL_deInit(Unit *unit)
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
