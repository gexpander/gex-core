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
    assert_param(driver != NULL);
    assert_param(driver->description != NULL);
    assert_param(driver->name != NULL);
    assert_param(driver->preInit != NULL);
    assert_param(driver->cfgLoadBinary != NULL);
    assert_param(driver->cfgLoadIni != NULL);
    assert_param(driver->cfgWriteBinary != NULL);
    assert_param(driver->cfgWriteIni != NULL);
    assert_param(driver->init != NULL);
    assert_param(driver->deInit != NULL);
    assert_param(driver->handleRequest != NULL);

    UregEntry *re = malloc_s(sizeof(UregEntry));
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

    pUnit->driver->deInit(pUnit);
    // Name is not expected to be freed by the deInit() function
    // - was alloc'd in the settings load loop
    if (isDynAlloc(pUnit->name)) {
        dbg("Freeing allocated name");
        free((void *) pUnit->name);
        pUnit->name = NULL;
    }
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
    bool suc = true;

    // Find type in the repository
    UregEntry *re = ureg_head;
    while (re != NULL) {
        if (streq(re->driver->name, driver_name)) {
            // Create new list entry
            UlistEntry *le = malloc_ck(sizeof(UlistEntry), &suc);
            CHECK_SUC();

            le->next = NULL;

            Unit *pUnit = &le->unit;
            pUnit->driver = re->driver;
            pUnit->status = E_LOADING;
            pUnit->data = NULL;
            pUnit->callsign = 0;

            suc = pUnit->driver->preInit(pUnit);
            if (!suc) {
                // tear down what we already allocated and abort

                // If it failed this early, the only plausible explanation is failed malloc,
                // in which case the data structure is not populated and keeping the
                // broken unit doesn't serve any purpose. Just ditch it...

                dbg("!! Unit type %s failed to pre-init!", driver_name);
                clean_failed_unit(pUnit);
                free(le);
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
                pUnit->name = strdup(typebuf);
                assert_param(pUnit->name);

                // callsign
                pUnit->callsign = pp_u8(pp);
                assert_param(pUnit->callsign != 0);

                // Load the rest of the unit
                pUnit->driver->cfgLoadBinary(pUnit, pp);
                assert_param(pp->ok);

                dbg("Adding unit \"%s\" of type %s", pUnit->name, pUnit->driver->name);

                suc = pUnit->driver->init(pUnit); // finalize the load and init the unit
                if (pUnit->status == E_LOADING) {
                    pUnit->status = suc ? E_SUCCESS : E_BAD_CONFIG;
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
        free(le);

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
                    name = strndup(p, delim - p);
                    p = delim + 1;
                } else {
                    // last name
                    name = strdup(p);
                    p = NULL; // quit after this loop ends
                }
                assert_param(name);
                Unit *pUnit = ureg_instantiate(driver_name);
                if (!pUnit) {
                    free(name);
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
bool ureg_load_unit_ini_key(const char *restrict name,
                            const char *restrict key,
                            const char *restrict value,
                            uint8_t callsign)
{
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        if (streq(li->unit.name, name)) {
            Unit *const pUnit = &li->unit;
            pUnit->callsign = callsign;
            return pUnit->driver->cfgLoadIni(pUnit, key, value);
        }

        li = li->next;
    }
    return false;
}

/** Finalize untis init */
bool ureg_finalize_all_init(void)
{
    dbg("Finalizing units init...");
    bool suc = true;
    UlistEntry *li = ulist_head;
    uint8_t callsign = 1;
    while (li != NULL) {
        Unit *const pUnit = &li->unit;

        bool s = pUnit->driver->init(pUnit);
        if (!s) {
            dbg("!!!! error initing unit %s", pUnit->name);
            if (pUnit->status == E_LOADING) {
                // assume it's a config error if not otherwise specified
                pUnit->status = E_BAD_CONFIG;
            }
        } else {
            pUnit->status = E_SUCCESS;
        }

        // try to assign unique callsigns
        if (pUnit->callsign == 0) {
            pUnit->callsign = callsign++;
        } else {
            if (pUnit->callsign >= callsign) {
                callsign = (uint8_t) (pUnit->callsign + 1);
            }
        }

        suc &= s;
        li = li->next;
    }
    return suc;
}

static void export_unit_do(UlistEntry *li, IniWriter *iw)
{
    Unit *const pUnit = &li->unit;

    iw_section(iw, "%s:%s@%d", pUnit->driver->name, pUnit->name, (int)pUnit->callsign);
    if (pUnit->status != E_SUCCESS) {
        iw_comment(iw, "!!! %s", error_get_string(pUnit->status));
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
    iw_comment(iw, "Create units by adding their names next to a type (e.g. PIN=A,B),");
    iw_comment(iw, "remove the same way. Reload to update the unit sections below.");

    // This could certainly be done in some more efficient way ...
    re = ureg_head;
    while (re != NULL) {
        // Should produce something like:

        // # Description string here
        // TYPE_ID=NAME1,NAME2
        //

        const UnitDriver *const pDriver = re->driver;

        iw_cmt_newline(iw);
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

/** Deliver message to it's destination unit */
void ureg_deliver_unit_request(TF_Msg *msg)
{
    PayloadParser pp = pp_start(msg->data, msg->len, NULL);
    uint8_t callsign = pp_u8(&pp);
    uint8_t command = pp_u8(&pp);

    // highest bit indicates user wants an extra confirmation on success
    bool confirmed = (bool) (command & 0x80);
    command &= 0x7F;

    if (!pp.ok) { dbg("!! pp not OK!"); }

    if (callsign == 0 || !pp.ok) {
        com_respond_malformed_cmd(msg->frame_id);
        return;
    }

    UlistEntry *li = ulist_head;
    while (li != NULL) {
        Unit *const pUnit = &li->unit;
        if (pUnit->callsign == callsign && pUnit->status == E_SUCCESS) {
            bool ok = pUnit->driver->handleRequest(pUnit, msg->frame_id, command, &pp);

            // send extra SUCCESS confirmation message.
            // error is expected to have already been reported.
            if (ok && confirmed) {
                com_respond_ok(msg->frame_id);
            }
            return;
        }
        li = li->next;
    }

    // Not found
    com_respond_snprintf(msg->frame_id, MSG_ERROR, "NO UNIT @ %"PRIu8, callsign);
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

    bool suc = true;
    uint8_t *buff = malloc_ck(msglen, &suc);
    if (!suc) { com_respond_str(MSG_ERROR, frame_id, "OUT OF MEMORY"); return; }
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
    free(buff);
}
