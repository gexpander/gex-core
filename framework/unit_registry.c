//
// Created by MightyPork on 2017/11/26.
//

#include <utils/hexdump.h>
#include "platform.h"
#include "utils/avrlibc.h"
#include "comm/messages.h"
#include "utils/ini_writer.h"
#include "utils/str_utils.h"
#include "utils/malloc_safe.h"
#include "unit_registry.h"
#include "resources.h"

// ** Unit repository **

typedef struct ureg_entry UregEntry;
typedef struct ulist_entry UlistEntry;

struct ureg_entry {
    const UnitDriver *driver;
    UregEntry *next;
};

UregEntry *ureg_head = NULL;
UregEntry *ureg_tail = NULL;

// ---

struct ulist_entry {
    Unit unit;
    UlistEntry *next;
};

UlistEntry *ulist_head = NULL;
UlistEntry *ulist_tail = NULL;

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

/** Remove a unit and update links appropriately */
static void remove_unit_from_list(UlistEntry *restrict le, UlistEntry *restrict parent)
{
    if (parent == NULL) {
        ulist_head = le->next;
    } else {
        parent->next = le->next;
    }

    // Fix tail potentially pointing to the removed entry
    if (ulist_tail == le) {
        ulist_tail = parent;
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

/** Find a unit in the list */
static UlistEntry *find_unit(const Unit *unit, UlistEntry **pParent)
{
    UlistEntry *le = ulist_head;
    UlistEntry *parent = NULL;
    while (le != NULL) {
        if (&le->unit == unit) {
            if (pParent != NULL) {
                *pParent = parent;
            }
            return le;
        }

        parent = le;
        le = le->next;
    }

    dbg("!! Unit was not found in registry");
    *pParent = NULL;
    return NULL;
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

// remove before init()
void ureg_clean_failed(Unit *unit)
{
    dbg("Removing failed unit from registry...");

    UlistEntry *le;
    UlistEntry *parent;
    le = find_unit(unit, &parent);
    if (!le) return;

    clean_failed_unit(&le->unit);

    remove_unit_from_list(le, parent);

    free(le);
}

// remove after successful init()
void ureg_remove_unit(Unit *unit)
{
    dbg("Cleaning & removing unit from registry...");

    UlistEntry *le;
    UlistEntry *parent;
    le = find_unit(unit, &parent);
    if (!le) return;

    free_le_unit(le);

    remove_unit_from_list(le, parent);
    free(le);
}

void ureg_save_units(PayloadBuilder *pb)
{
    assert_param(pb->ok);

    uint32_t count = ureg_get_num_units();

    pb_char(pb, 'U');
    pb_u16(pb, (uint16_t) count);

    UlistEntry *le = ulist_head;
    while (le != NULL) {
        Unit *const pUnit = &le->unit;
        pb_char(pb, 'u');
        pb_string(pb, pUnit->driver->name);
        pb_string(pb, pUnit->name);
        pb_u8(pb, pUnit->callsign);

        // Now all the rest, unit-specific
        pUnit->driver->cfgWriteBinary(pUnit, pb);
        assert_param(pb->ok);
        le = le->next;
    }
}

bool ureg_load_units(PayloadParser *pp)
{
    bool suc;
    char typebuf[16];

    assert_param(pp->ok);

    if (pp_char(pp) != 'U') return false;
    uint16_t unit_count = pp_u16(pp);
    dbg("Units to load: %d", (int)unit_count);

    for (uint32_t j = 0; j < unit_count; j++) {
        // We're now unpacking a single unit

        // Marker that this is a unit - it could get out of alignment if structure changed
        if (pp_char(pp) != 'u') return false;

        // TYPE
        pp_string(pp, typebuf, 16);
        Unit *const pUnit = ureg_instantiate(typebuf);
        assert_param(pUnit);

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
    }

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
}


bool ureg_instantiate_by_ini(const char *restrict driver_name, const char *restrict names)
{
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

bool ureg_load_unit_ini_key(const char *restrict name,
                            const char *restrict key,
                            const char *restrict value)
{
    UlistEntry *li = ulist_head;
    while (li != NULL) {
        if (streq(li->unit.name, name)) {
            Unit *const pUnit = &li->unit;

            if (streq(key, "callsign")) {
                // handled separately from unit data
                pUnit->callsign = (uint8_t) avr_atoi(value);
                return true;
            } else {
                return pUnit->driver->cfgLoadIni(pUnit, key, value);
            }
        }

        li = li->next;
    }
    return false;
}

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

    iw_section(iw, "%s:%s", pUnit->driver->name, pUnit->name);
    if (pUnit->status != E_SUCCESS) {
        iw_comment(iw, "!!! %s", error_get_string(pUnit->status));
    }
    iw_newline(iw);

    iw_comment(iw, "Unit address 1-255");
    iw_entry(iw, "callsign", "%d", pUnit->callsign);

    pUnit->driver->cfgWriteIni(pUnit, iw);
    iw_newline(iw);
}

// unit to INI
void ureg_export_unit(uint32_t index, IniWriter *iw)
{
    UlistEntry *li = ulist_head;
    uint32_t count = 0;
    while (li != NULL) {
        if (count == index) {
            export_unit_do(li, iw);
            return;
        }

        count++;
        li = li->next;
    }
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

        iw_newline(iw);
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
        }
        re = re->next;
        iw_newline(iw);
    }
    iw_newline(iw); // space before the unit sections

    // Now we dump all the units
    li = ulist_head;
    while (li != NULL) {
        export_unit_do(li, iw);
        li = li->next;
    }
}

// count units
uint32_t ureg_get_num_units(void)
{
    // TODO keep this in a variable
    UlistEntry *li = ulist_head;
    uint32_t count = 0;
    while (li != NULL) {
        count++;
        li = li->next;
    }

    return count;
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
