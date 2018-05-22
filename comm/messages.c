//
// Created by MightyPork on 2017/11/21.
//

#include "platform.h"
#include "framework/settings.h"
#include "utils/ini_parser.h"
#include "TinyFrame.h"
#include "framework/unit_registry.h"
#include "comm/messages.h"
#include "framework/system_settings.h"
#include "utils/malloc_safe.h"
#include "platform/status_led.h"
#include "interfaces.h"

static TinyFrame tf_;
TinyFrame *comm = &tf_;

// ---------------------------------------------------------------------------

/**
 * Ping request listener - returns version string and other info about the build
 */
static TF_Result lst_ping(TinyFrame *tf, TF_Msg *msg)
{
    dbg("Ping!");
    com_respond_snprintf(msg->frame_id, MSG_SUCCESS,
                         "GEX v%s on %s (%s)", GEX_VERSION, GEX_PLATFORM, COMPORT_NAMES[gActiveComport]);
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

/** Callback for bulk read of a settings file */
static void ini_bulkread_cb(BulkRead *bulk, uint32_t chunk, uint8_t *buffer)
{
    // clean-up request
    if (buffer == NULL) {
        free_ck(bulk);
        iw_end();
        return;
    }

    if (bulk->offset == 0) iw_begin();

    IniWriter iw = iw_init((char *)buffer, bulk->offset, chunk);
    iw.tag = 1; // indicates this is read via the API (affects some comments)

    uint8_t filenum = (uint8_t) (int) bulk->userdata;
    
    if (filenum == 0) {
        settings_build_units_ini(&iw);
    }
    else if (filenum == 1) {
        settings_build_system_ini(&iw);
    }
}

/**
 * Listener: Export a file via TF
 */
static TF_Result lst_ini_export(TinyFrame *tf, TF_Msg *msg)
{
//    dbg("Bulk read INI file");

    BulkRead *bulk = malloc_ck(sizeof(BulkRead));
    assert_param(bulk != NULL);

    uint8_t filenum = 0;

    // if any payload, the first byte defines the file to read
    // 0 - units
    // 1 - system
    // (this is optional for backwards compatibility)
    if (msg->len > 0) {
        filenum = msg->data[0];
    }

    bulk->frame_id = msg->frame_id;
    bulk->read = ini_bulkread_cb;
    bulk->userdata = (void *) (int)filenum;

    if (filenum == 0) {
        bulk->len = iw_measure_total(settings_build_units_ini, 1);
    }
    else if (filenum == 1) {
        bulk->len = iw_measure_total(settings_build_system_ini, 1);
    }

    bulkread_start(tf, bulk);
    Indicator_Effect(STATUS_DISK_BUSY_SHORT);
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
//            dbg("INI write complete");
        } else {
            dbg("INI write failed");
        }

        free_ck(bulk);
        return;
    }

    ini_parse((const char *) chunk, len);
}

/**
 * Listener: Import INI file (write from PC via TF)
 */
static TF_Result lst_ini_import(TinyFrame *tf, TF_Msg *msg)
{
//    dbg("Bulk write INI file");

    BulkWrite *bulk = malloc_ck(sizeof(BulkWrite));
    assert_param(bulk);

    bulk->frame_id = msg->frame_id;

    // we get the total len in the payload - ini can be as long as it wants, we parse it on-line
    PayloadParser pp = pp_start(msg->data, msg->len, NULL);
    uint32_t len = pp_u32(&pp);
    if (!pp.ok) {
        com_respond_error(msg->frame_id, E_MALFORMED_COMMAND);
        goto done;
    }

    bulk->len = len;
    bulk->write = settings_bulkwrite_cb;

    settings_load_ini_begin();
    ini_parse_begin(iniparser_cb, NULL);
    bulkwrite_start(tf, bulk);
    Indicator_Effect(STATUS_DISK_BUSY);
done:
    return TF_STAY;
}

// ---------------------------------------------------------------------------

/** Listener: Save settings to Flash */
static TF_Result lst_persist_cfg(TinyFrame *tf, TF_Msg *msg)
{
    Indicator_Effect(STATUS_DISK_REMOVED);
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
