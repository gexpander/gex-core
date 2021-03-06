//
// Created by MightyPork on 2017/11/26.
//

#include "platform.h"
#include "utils/hexdump.h"
#include "utils/avrlibc.h"
#include "comm/messages.h"
#include "utils/ini_writer.h"
#include "utils/str_utils.h"
#include "utils/malloc_safe.h"
#include "unit_registry.h"
#include "system_settings.h"
#include "resources.h"

// ** Unit repository **

typedef struct ureg_entry UregEntry;
typedef struct ulist_entry UlistEntry;

struct ureg_entry {
    const UnitDriver *driver;
    UregEntry *next;
};

static UregEntry *ureg_head = NULL;
static UregEntry *ureg_tail = NULL;

// ---

struct ulist_entry {
    Unit unit;
    UlistEntry *next;
};

static UlistEntry *ulist_head = NULL;
static UlistEntry *ulist_tail = NULL;

static int32_t unit_count = -1;

// ---


void ureg_add_type(const UnitDriver *driver)
{
    bool suc = true;
    assert_param(driver != NULL);
    assert_param(driver->name != NULL);

    dbg("Loading driver %s", driver->name);

    assert_param(driver->description != NULL);
    assert_param(driver->preInit != NULL);
    assert_param(driver->cfgLoadBinary != NULL);
    assert_param(driver->cfgLoadIni != NULL);
    assert_param(driver->cfgWriteBinary != NULL);
    assert_param(driver->cfgWriteIni != NULL);
    assert_param(driver->init != NULL);
    assert_param(driver->deInit != NULL);
    assert_param(driver->handleRequest != NULL);

    UregEntry *re = calloc_ck(1, sizeof(UregEntry));
    assert_param(re != NULL);

    re->driver = driver;
    re->next = NULL;

    if (ureg_head == NULL) {
        ureg_head = re;
    } else {
        ureg_tail->next = re;
    }
    ureg_tail = re;
}


/** Free unit in a list entry (do not free the list entry itself!) */
static void free_le_unit(UlistEntry *le)
{
    Unit *const pUnit = &le->unit;

    assert_param(pUnit->data);
    pUnit->driver->deInit(pUnit);
    // Name is not expected to be freed by the deInit() function
    // - was alloc'd in the settings load loop
    free_ck(pUnit->name);
}


/** Add unit to the list, updating references as needed */
static void add_unit_to_list(UlistEntry *le)
{
    // Attach to the list
    if (ulist_head == NULL) {
        ulist_head = le;
    } else {
        ulist_tail->next = le;
    }
    ulist_tail = le;
}


// create a unit instance (not yet loading or initing - just pre-init)
Unit *ureg_instantiate(const char *driver_name)
{
    error_t rv;

    // Find type in the repository
    UregEntry *re = ureg_head;
    while (re != NULL) {
        if (streq(re->driver->name, driver_name)) {
            // Create new list entry
            UlistEntry *le = calloc_ck(1, sizeof(UlistEntry));
            if (le == NULL) return NULL;

            le->next = NULL;

            Unit *pUnit = &le->unit;
            pUnit->driver = re->driver;
            pUnit->status = E_LOADING; // indeterminate default state
            pUnit->data = NULL;
            pUnit->callsign = 0;

            rv = pUnit->driver->preInit(pUnit);
            if (rv != E_SUCCESS) {
                // tear down what we already allocated and abort

                // If it failed this early, the only plausible explanation is failed malloc,
                // in which case the data structure is not populated and keeping the
                // broken unit doesn't serve any purpose. Just ditch it...

                dbg("!! Unit type %s failed to pre-init! %s", driver_name, error_get_message(rv));
                clean_failed_unit(pUnit);
                free_ck(le);
                return NULL;
            }

            add_unit_to_list(le);
            return pUnit;
        }
        re = re->next;
    }

    dbg("!! Did not find unit type %s", driver_name);
    return NULL;
}


void ureg_save_units(PayloadBuilder *pb)
{
    assert_param(pb->ok);

    uint32_t count = ureg_get_num_units();

    pb_char(pb, 'U');
    pb_u8(pb, 0); // Format version

    { // Units list
        pb_u16(pb, (uint16_t) count);

        UlistEntry *le = ulist_head;
        while (le != NULL) {
            Unit *const pUnit = &le->unit;
            pb_char(pb, 'u'); // marker
            { // Single unit
                pb_string(pb, pUnit->driver->name);
                pb_string(pb, pUnit->name);
                pb_u8(pb, pUnit->callsign);

                // Now all the rest, unit-specific
                pUnit->driver->cfgWriteBinary(pUnit, pb);
                assert_param(pb->ok);
            } // end single unit
            le = le->next;
        }
    } // end units list
}


bool ureg_load_units(PayloadParser *pp)
{
    bool suc;
    char typebuf[16];

    assert_param(pp->ok);
    unit_count = -1; // reset the counter

    // Check units list marker byte
    if (pp_char(pp) != 'U') return false;
    uint8_t version = pp_u8(pp); // units list format version

    (void)version; // version can affect the format

    { // units list
        uint16_t count = pp_u16(pp);
        dbg("Units to load: %d", (int) count);

        for (uint32_t j = 0; j < count; j++) {
            // We're now unpacking a single unit

            // Marker that this is a unit - it could get out of alignment if structure changed
            if (pp_char(pp) != 'u') return false;

            { // Single unit
                // TYPE
                pp_string(pp, typebuf, 16);
                Unit *const pUnit = ureg_instantiate(typebuf);
                if (!pUnit) {
                    dbg("!! Unknown unit type %s, aborting load.", typebuf);
                    break;
                }

                // NAME
                pp_string(pp, typebuf, 16);
                pUnit->name = strdup_ck(typebuf);
                assert_param(pUnit->name);

                // callsign
                pUnit->callsign = pp_u8(pp);
                assert_param(pUnit->callsign != 0);

                // Load the rest of the unit
                pUnit->driver->cfgLoadBinary(pUnit, pp);
                assert_param(pp->ok);

                dbg("Adding unit \"%s\" of type %s", pUnit->name, pUnit->driver->name);

                pUnit->status = pUnit->driver->init(pUnit); // finalize the load and init the unit
                if (pUnit->status != E_SUCCESS) {
                    dbg("!!! error initing unit %s: %s", pUnit->name, error_get_message(pUnit->status));
                }
            } // end unit
        }
    } // end units list

    return pp->ok;
}


void ureg_remove_all_units(void)
{
    UlistEntry *le = ulist_head;
    UlistEntry *next;

    dbg("Removing all units");

    while (le != NULL) {
        next = le->next;

        free_le_unit(le);
        free_ck(le);

        le = next;
    }

    ulist_head = ulist_tail = NULL;
    unit_count = -1;
}


/** Create unit instances from the [UNITS] overview section */
bool ureg_instantiate_by_ini(const char *restrict driver_name, const char *restrict names)
{
    unit_count = -1; // reset the counter

    UregEntry *re = ureg_head;
    while (re != NULL) {
        if (streq(re->driver->name, driver_name)) {
            const char *p = names;
            while (p != NULL) { // we use this to indicate we're done
                // skip leading whitespace (assume there's never whitespace before a comma)
                while (*p == ' ' || *p == '\t') p++;
                if (*p == 0) break; // out of characters

                const char *delim = strchr(p, ',');
                char *name = NULL;
                if (delim != NULL) {
                    // not last
                    name = strndup_ck(p, delim - p + 1);
                    p = delim + 1;
                } else {
                    // last name
                    name = strdup_ck(p);
                    p = NULL; // quit after this loop ends
                }
                assert_param(name);
                Unit *pUnit = ureg_instantiate(driver_name);
                if (!pUnit) {
                    free_ck(name);
                    return false;
                }

                pUnit->name = name;

                // don't init yet - leave that for when we're done with the INI
            }

            return true;
        }
        re = re->next;
    }

    dbg("! ureg instantiate - bad type");
    return false;
}


/** Load unit key-value */
error_t ureg_load_unit_ini_key(const char *restrict name,
                            const char *restrict key,
                            const char *restrict value,
                            uint8_t callsign)
{
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        Unit *const pUnit = &li->unit;
        if (streq(pUnit->name, name)) {
            pUnit->callsign = callsign;
            return pUnit->driver->cfgLoadIni(pUnit, key, value);
        }

        li = li->next;
    }
    return E_NO_SUCH_UNIT;
}


/** Finalize units init. Returns true if all inited OK. */
bool ureg_finalize_all_init(void)
{
    dbg("Finalizing units init...");
    bool suc = true;
    UlistEntry *li = ulist_head;
    uint8_t callsign = 1;
    while (li != NULL) {
        Unit *const pUnit = &li->unit;
        dbg("+ %s \"%s\"", pUnit->driver->name, pUnit->name);

        if (pUnit->status == E_SUCCESS) {
            dbg("! Unit seems already loaded, skipping");
        } else {
            pUnit->status = pUnit->driver->init(pUnit);
            if (pUnit->status != E_SUCCESS) {
                dbg("!!! error initing unit %s: %s", pUnit->name, error_get_message(pUnit->status));
            }

            // try to assign unique callsigns
            if (pUnit->callsign == 0) {
                // this is very inefficient but should be reliable
                bool change;
                do {
                    change = false;
                    UlistEntry *xli = ulist_head;
                    while (xli != NULL) {
                        if (xli->unit.callsign != 0) {
                            if (xli->unit.callsign == callsign) {
                                change = true;
                                callsign++;
                            }
                        }
                        xli = xli->next;
                    }
                } while (change && callsign < 255);
                pUnit->callsign = callsign;
            }
        }

        suc &= (pUnit->status == E_SUCCESS);
        li = li->next;
    }
    return suc;
}

/** helper foir ureg_build_ini() */
static void export_unit_do(UlistEntry *li, IniWriter *iw)
{
    Unit *const pUnit = &li->unit;

    iw_section(iw, "%s:%s@%d", pUnit->driver->name, pUnit->name, (int)pUnit->callsign);
    if (pUnit->status != E_SUCCESS) {
        // temporarily force comments ON
        bool sc = SystemSettings.ini_comments;
        SystemSettings.ini_comments = true;
        {
            // special message for failed unit die to resource
            if (pUnit->status == E_RESOURCE_NOT_AVAILABLE) {
                iw_commentf(iw, "!!! %s not available, already held by %s",
                           rsc_get_name(pUnit->failed_rsc),
                           rsc_get_owner_name(pUnit->failed_rsc));
            }
            else {
                iw_commentf(iw, "!!! %s", error_get_message(pUnit->status));
            }
            iw_cmt_newline(iw);
        }
        SystemSettings.ini_comments = sc;
    }
    pUnit->driver->cfgWriteIni(pUnit, iw);
}


// unit to INI
void ureg_build_ini(IniWriter *iw)
{
    UlistEntry *li;
    UregEntry *re;

    // Unit list
    iw_section(iw, "UNITS");
    iw_comment(iw, "Create units by adding their names next to a type (e.g. DO=A,B),");
    iw_comment(iw, "remove the same way. Reload to update the unit sections below.");
    iw_cmt_newline(iw);

    // This could certainly be done in some more efficient way ...
    re = ureg_head;
    while (re != NULL) {
        // Should produce something like:

        // # Description string here
        // TYPE_ID=NAME1,NAME2
        //

        const UnitDriver *const pDriver = re->driver;

        iw_comment(iw, pDriver->description);
        iw_string(iw, pDriver->name);
        iw_string(iw, "=");

        li = ulist_head;
        uint32_t count = 0;
        while (li != NULL) {
            Unit *const pUnit = &li->unit;
            if (streq(pUnit->driver->name, pDriver->name)) {
                if (count > 0) iw_string(iw, ",");
                iw_string(iw, pUnit->name);
                count++;
            }
            li = li->next;

            if (iw->count == 0) return; // avoid printing discarded tail
        }
        re = re->next;
        iw_newline(iw);
    }

    // Now we dump all the units
    li = ulist_head;
    while (li != NULL) {
        export_unit_do(li, iw);
        li = li->next;

        if (iw->count == 0) return; // avoid printing discarded tail
    }
}


// count units
uint32_t ureg_get_num_units(void)
{
    if (unit_count == -1) {
        // TODO keep this in a variable
        UlistEntry *li = ulist_head;
        uint32_t count = 0;
        while (li != NULL) {
            count++;
            li = li->next;
        }
        unit_count = count;
    }

    return (uint32_t) unit_count;
}


extern osMutexId mutScratchBufferHandle;

/** Deliver message to it's destination unit */
void ureg_deliver_unit_request(TF_Msg *msg)
{
    // we must claim the scratch buffer because it's used by many units internally
    assert_param(osOK == osMutexWait(mutScratchBufferHandle, 5000));
    {
        PayloadParser pp = pp_start(msg->data, msg->len, NULL);
        uint8_t callsign = pp_u8(&pp);
        uint8_t command = pp_u8(&pp);

        // highest bit indicates user wants an extra confirmation on success
        bool confirmed = (bool) (command & 0x80);
        command &= 0x7F;

        if (callsign == 0 || !pp.ok) {
            com_respond_error(msg->frame_id, E_MALFORMED_COMMAND);
            goto quit;
        }

        UlistEntry *li = ulist_head;
        while (li != NULL) {
            Unit *const pUnit = &li->unit;
            if (pUnit->callsign == callsign && pUnit->status == E_SUCCESS) {
                error_t rv = pUnit->driver->handleRequest(pUnit, msg->frame_id, command, &pp);

                if (!pp.ok) {
                    com_respond_error(msg->frame_id, E_MALFORMED_COMMAND);
                    goto quit;
                }

                // send extra SUCCESS confirmation message.
                // error is expected to have already been reported.
                if (rv == E_SUCCESS) {
                    if (confirmed) com_respond_ok(msg->frame_id);
                }
                else if (rv != E_FAILURE) {
                    // Failure is returned when the handler already sent an error response.
                    com_respond_error(msg->frame_id, rv);
                }
                goto quit;
            }
            li = li->next;
        }

        // Not found
        com_respond_error(msg->frame_id, E_NO_SUCH_UNIT);
    }
    quit:
    assert_param(osOK == osMutexRelease(mutScratchBufferHandle));
}


/** Send a response for a unit-list request */
void ureg_report_active_units(TF_ID frame_id)
{
    // count bytes needed
    uint32_t msglen = 1; // for the count byte

    UlistEntry *li = ulist_head;
    uint32_t count = 0;
    while (li != NULL) {
        if (li->unit.status == E_SUCCESS) {
            count++;
            msglen += strlen(li->unit.name) + 1;
            msglen += strlen(li->unit.driver->name) + 1;
        }
        li = li->next;
    }
    msglen += count; // one byte per message for the callsign

    uint8_t *buff = calloc_ck(1, msglen);
    if (buff == NULL) {
        com_respond_error(frame_id, E_OUT_OF_MEM);
        return;
    }

    {
        PayloadBuilder pb = pb_start(buff, msglen, NULL);
        pb_u8(&pb, (uint8_t) count); // assume we don't have more than 255 units

        li = ulist_head;
        while (li != NULL) {
            if (li->unit.status == E_SUCCESS) {
                pb_u8(&pb, li->unit.callsign);
                pb_string(&pb, li->unit.name);
                pb_string(&pb, li->unit.driver->name);
            }
            li = li->next;
        }

        assert_param(pb.ok);

        com_respond_buf(frame_id, MSG_SUCCESS, buff, msglen);
    }
    free_ck(buff);
}


Unit *ureg_get_rsc_owner(Resource resource)
{
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        if (RSC_IS_HELD(li->unit.resources, resource)) {
            return &li->unit;
        }
        li = li->next;
    }

    if (RSC_IS_HELD(UNIT_SYSTEM.resources, resource)) return &UNIT_SYSTEM;
    if (RSC_IS_HELD(UNIT_PLATFORM.resources, resource)) return &UNIT_PLATFORM;

    return NULL;
}


void ureg_print_unit_resources(IniWriter *iw)
{
    if (iw->count == 0) return;

    iw_string(iw, "Resources held by units\r\n"
                  "-----------------------\r\n");
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        iw_string(iw, li->unit.name);
        iw_string(iw, ": ");

        bool first = true;
        for (uint32_t rsc = 0; rsc < RESOURCE_COUNT; rsc++) {
            if (!RSC_IS_HELD(li->unit.resources, (Resource)rsc)) continue;
            if (!first) iw_string(iw, ", ");
            iw_string(iw, rsc_get_name((Resource) rsc));
            first = false;
        }
        if (first) iw_string(iw, "-none-");
        iw_newline(iw);

        li = li->next;
    }
    iw_newline(iw);
}


void ureg_tick_units(void)
{
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        Unit *const pUnit = &li->unit;
        if (pUnit && pUnit->data && pUnit->status == E_SUCCESS && (pUnit->tick_interval > 0 || pUnit->_tick_cnt > 0)) {
            if (pUnit->_tick_cnt > 0) {
                pUnit->_tick_cnt--; // check for 0 allows one-off timers
            }

            if (pUnit->_tick_cnt == 0) {
                if (pUnit->driver->updateTick) {
                    pUnit->driver->updateTick(pUnit);
                }
                pUnit->_tick_cnt = pUnit->tick_interval;
            }
        }
        li = li->next;
    }
}
