//
// Created by MightyPork on 2017/11/21.
//

#include <framework/settings.h>
#include "platform.h"
#include "TinyFrame.h"
#include "framework/unit_registry.h"
#include "comm/messages.h"

static TinyFrame tf_;
TinyFrame *comm = &tf_;

// ---------------------------------------------------------------------------

/**
 * Ping request listener - returns version string and other info about the build
 */
static TF_Result lst_ping(TinyFrame *tf, TF_Msg *msg)
{
    com_respond_snprintf(msg->frame_id, MSG_SUCCESS, "%s/%s", GEX_VERSION, GEX_PLATFORM);
    return TF_STAY;
}

/**
 * Default listener, fallback for unhandled messages
 */
static TF_Result lst_default(TinyFrame *tf, TF_Msg *msg)
{
    dbg("!! Unhandled msg type %02"PRIx8", frame_id 0x%04"PRIx16, msg->type, msg->frame_id);

    com_respond_snprintf(msg->frame_id, MSG_ERROR, "UNKNOWN MSG %"PRIu8, msg->type);
    return TF_STAY;
}

/**
 * Unit request listener, a message targeted at a particular
 */
static TF_Result lst_unit(TinyFrame *tf, TF_Msg *msg)
{
    ureg_deliver_unit_request(msg);
    return TF_STAY;
}

/**
 * List all active unit callsigns, names and types
 */
static TF_Result lst_list_units(TinyFrame *tf, TF_Msg *msg)
{
    ureg_report_active_units(msg->frame_id);
    return TF_STAY;
}

// ---------------------------------------------------------------------------

/** Callback for bulk read of the settings file */
static void settings_bulkread_cb(BulkRead *bulk, uint32_t chunk, uint8_t *buffer)
{
    // clean-up request
    if (buffer == NULL) {
        free(bulk);
        return;
    }

    IniWriter iw = iw_init((char *)buffer, bulk->offset, chunk);
    settings_build_ini(&iw);
}

static TF_Result lst_ini_export(TinyFrame *tf, TF_Msg *msg)
{
    BulkRead *bulk = malloc(sizeof(BulkRead));
    bulk->frame_id = msg->frame_id;
    bulk->len = settings_get_ini_len();
    bulk->read = settings_bulkread_cb;
    bulkread_start(tf, bulk);

    return TF_STAY;
}

static TF_Result lst_ini_import(TinyFrame *tf, TF_Msg *msg)
{
    //
    return TF_STAY;
}

// ---------------------------------------------------------------------------

void comm_init(void)
{
    TF_InitStatic(comm, TF_SLAVE);

    bool suc = true;

    suc &= TF_AddTypeListener(comm, MSG_PING, lst_ping);
    suc &= TF_AddTypeListener(comm, MSG_UNIT_REQUEST, lst_unit);
    suc &= TF_AddTypeListener(comm, MSG_LIST_UNITS, lst_list_units);
    suc &= TF_AddTypeListener(comm, MSG_INI_READ, lst_ini_export);
    suc &= TF_AddTypeListener(comm, MSG_INI_WRITE, lst_ini_import);

    // fall-through
    suc &= TF_AddGenericListener(comm, lst_default);

    assert_param(suc);
}
