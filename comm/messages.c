//
// Created by MightyPork on 2017/11/21.
//

#include <framework/system_settings.h>
#include "platform.h"
#include "framework/settings.h"
#include "utils/ini_parser.h"
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
    com_respond_snprintf(msg->frame_id, MSG_SUCCESS,
                         "GEX v%s on %s", GEX_VERSION, GEX_PLATFORM);
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
        iw_end();
        dbg("INI read complete.");
        return;
    }

    if (bulk->offset == 0) iw_begin();

    IniWriter iw = iw_init((char *)buffer, bulk->offset, chunk);
    settings_build_units_ini(&iw);
}

/**
 * Listener: Export INI file via TF
 */
static TF_Result lst_ini_export(TinyFrame *tf, TF_Msg *msg)
{
    dbg("Bulk read INI file");

    BulkRead *bulk = malloc(sizeof(BulkRead));
    assert_param(bulk);

    bulk->frame_id = msg->frame_id;
    bulk->len = iw_measure_total(settings_build_units_ini);
    bulk->read = settings_bulkread_cb;
    bulk->userdata = NULL;

    bulkread_start(tf, bulk);

    return TF_STAY;
}

// ---------------------------------------------------------------------------

/** Callback for receiving a key-value pair from the INi parser when importing a INI file */
static void iniparser_cb(const char *section, const char *key, const char *value, void *userData)
{
    settings_load_ini_key(section, key, value);
}

/** Callback for importing INI file */
static void settings_bulkwrite_cb(BulkWrite *bulk, const uint8_t *chunk, uint32_t len)
{
    // clean-up request
    if (chunk == NULL) {
        ini_parse_end();

        if (bulk->offset > 0) {
            settings_load_ini_end();
            dbg("INI write complete");
        } else {
            dbg("INI write failed");
        }

        free(bulk);
        return;
    }

    ini_parse((const char *) chunk, len);
}

/**
 * Listener: Import INI file (write from PC via TF)
 */
static TF_Result lst_ini_import(TinyFrame *tf, TF_Msg *msg)
{
    dbg("Bulk write INI file");

    BulkWrite *bulk = malloc(sizeof(BulkWrite));
    assert_param(bulk);

    bulk->frame_id = msg->frame_id;

    // we get the total len in the payload - ini can be as long as it wants, we parse it on-line
    PayloadParser pp = pp_start(msg->data, msg->len, NULL);
    uint32_t len = pp_u32(&pp);
    if (!pp.ok) {
        com_respond_error(msg->frame_id, E_PROTOCOL_BREACH);
        goto done;
    }

    bulk->len = len;
    bulk->write = settings_bulkwrite_cb;

    settings_load_ini_begin();
    ini_parse_begin(iniparser_cb, NULL);

    bulkwrite_start(tf, bulk);

done:
    return TF_STAY;
}

// ---------------------------------------------------------------------------

/** Listener: Save settings to Flash */
static TF_Result lst_persist_cfg(TinyFrame *tf, TF_Msg *msg)
{
    settings_save();
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
    suc &= TF_AddTypeListener(comm, MSG_PERSIST_CFG, lst_persist_cfg);

    // fall-through
    suc &= TF_AddGenericListener(comm, lst_default);

    assert_param(suc);
}
