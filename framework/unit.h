//
// Created by MightyPork on 2017/11/24.
//

#ifndef GEX_UNIT_H
#define GEX_UNIT_H

#include "platform.h"
#include <TinyFrame.h>
#include "utils/ini_writer.h"
#include "utils/payload_builder.h"
#include "utils/payload_parser.h"

extern char unit_tmp64[64]; // temporary static buffer

typedef struct unit Unit;
typedef struct unit_driver UnitDriver;

struct unit {
    const UnitDriver *driver;

    /** Unit name (used in error messages) */
    const char *name;

    /**
     * Storage for arbitrary unit data (allocated in 'preInit' and freed in 'deInit' or when init/load fails)
     */
    void *data;

    /** Unit init status */
    error_t status;

    /** Unit call sign for messages */
    uint8_t callsign;
};

/**
 * Unit instance - statically or dynamically allocated (depends whether it's system or user unit)
 */
struct unit_driver {
    /** Driver ID */
    const char *name;

    /** Unit type description (for use in comments) */
    const char *description;

    /**
     * Pre-init: allocate data object, init defaults
     */
    bool (*preInit)(Unit *unit);

    /**
     * Load settings from binary storage, parse and store them in the data object.
     * Don't do any validation, that's left for the init() function
     *
     * @param pp - parser
     */
    void (*cfgLoadBinary)(Unit *unit, PayloadParser *pp);

    /**
     * Write settings to binary storage.
     *
     * @param pb - builder
     */
    void (*cfgWriteBinary)(Unit *unit, PayloadBuilder *pb);

    /**
     * Load settings from a INI file.
     * Name has already been parsed and assigned.
     * This function is called repeatedly as kv-pairs are encountered in the stream.
     *
     * @param key - key from the INI file
     * @param value - value from the ini file; strings have already removed quotes and replaced escape sequences with ASCII as needed
     */
    bool (*cfgLoadIni)(Unit *unit, const char *key, const char *value);

    /**
     * Export settings to a INI file.
     * Capacity will likely be 512 bytes, do not waste space!
     *
     * @param buffer - destination buffer
     * @param capacity - buffer size
     * @return nubmer of bytes used
     */
    void (*cfgWriteIni)(Unit *unit, IniWriter *iw);

    /**
     * Finalize the init sequence, validate settings, enable peripherals and prepare for operation
     */
    bool (*init)(Unit *unit);

    /**
     * De-initialize the unit: de-init peripheral, free resources, free data object...
     * This is called when disabling all units in order to reload new config.
     */
    void (*deInit)(Unit *unit);

    /**
     * Handle an incoming request. Return true if command was OK.
     */
    bool (*handleRequest)(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp);
};

/**
 * De-init a partially initialized unit (before 'init' succeeds)
 * This releases all held resources and frees the *data,
 * if it looks like it has been dynamically allocated.
 *
 * Does NOT free the unit struct itself
 *
 * @param unit - unit to discard
 */
void clean_failed_unit(Unit *unit);

/** Marks peripherals claimed by the system */
extern Unit UNIT_SYSTEM;
/** Marks peripherals not available on the platform */
extern Unit UNIT_PLATFORM;

#endif //GEX_UNIT_H
