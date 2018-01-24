//
// Created by MightyPork on 2017/11/24.
//
// Structures and basic scaffolding for defining unit drivers
//

#ifndef GEX_UNIT_H
#define GEX_UNIT_H

#include "platform.h"
#include <TinyFrame.h>
#include "utils/ini_writer.h"
#include "utils/payload_builder.h"
#include "utils/payload_parser.h"
#include "rsc_enum.h"

/** Helper macro that returns E_BAD_UNIT_TYPE if unit is not of the given type */
#define CHECK_TYPE(_unit, _driver) do { \
    if (((_unit)->driver) != (_driver)) \
        return E_BAD_UNIT_TYPE; \
} while (0)

/** Shared unit scratch buffer */
extern char unit_tmp512[UNIT_TMP_LEN]; // temporary static buffer - not expected to be accessed asynchronously
// TODO add mutex?

/** Unit typedef - a instance */
typedef struct unit Unit;

/** Unit driver typedef - type handlers / props object */
typedef struct unit_driver UnitDriver;

/**
 * Unit instance structure
 */
struct unit {
    /** Reference to the used driver */
    const UnitDriver *driver;

    /** Unit name (used in error messages) */
    const char *name;

    /**
     * Storage for arbitrary unit data (allocated in 'preInit' and freed in 'deInit' or when init/load fails)
     */
    void *data;

    /** Unit call sign for messages */
    uint8_t callsign;

    /** Unit init status */
    error_t status;

    /** If RSC not avail. error is caught, the resource is stored here. */
    Resource failed_rsc;

    /** Bit-map of held resources */
    ResourceMap resources;

    /** Tick interval to run the updateTick() function, if defined */
    uint16_t tick_interval;

    /** Current number of ticks since last interval completion */
    uint16_t _tick_cnt;
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
    error_t (*preInit)(Unit *unit);

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
    error_t (*cfgLoadIni)(Unit *unit, const char *key, const char *value);

    /**
     * Export settings to a INI file.
     *
     * @param buffer - destination buffer
     * @param capacity - buffer size
     * @return nubmer of bytes used
     */
    void (*cfgWriteIni)(Unit *unit, IniWriter *iw);

    /**
     * Finalize the init sequence, validate settings, enable peripherals and prepare for operation
     */
    error_t (*init)(Unit *unit);

    /**
     * De-initialize the unit: de-init peripheral, free resources, free data object...
     * This is called when disabling all units in order to reload new config.
     */
    void (*deInit)(Unit *unit);

    /**
     * Handle an incoming request. Return true if command was OK.
     */
    error_t (*handleRequest)(Unit *unit, TF_ID frame_id, uint8_t command, PayloadParser *pp);

    /**
     * Periodic update call.
     * This is run from the SysTick interrupt handler,
     * any communication should be deferred via the job queue.
     */
    void (*updateTick)(Unit *unit);
};

/**
 * De-init a partially initialized unit (before 'init' succeeds)
 * This releases all held resources and frees *data and *name.
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
