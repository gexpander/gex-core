//
// Created by MightyPork on 2017/11/26.
//

#ifndef GEX_UNIT_REGISTRY_H
#define GEX_UNIT_REGISTRY_H

#include <TinyFrame/TinyFrame.h>
#include "platform.h"
#include "unit.h"

/**
 * Add instantiable unit type to the registry
 *
 * @param driver - unit template, will be shallowly cloned for new instances
 */
void ureg_add_type(const UnitDriver *driver);

/**
 * Create an instance of a unit type. The unit is added to the unit list.
 *
 * @param driver_name - unit type, same as given when registering the type. CAN BE ON STACK! Not stored.
 * @return the unit, or NULL on failure
 */
Unit *ureg_instantiate(const char *driver_name);

/**
 * Clean a unit previously obtained by 'ureg_instantiate' that
 * failed to properly load or init. This tears it down and removes it from the unit list.
 *
 * @param unit - unit to remove
 */
void ureg_clean_failed(Unit *unit);

/**
 * Safely delete a unit instance (releasing memory and reosurces, removing it from the list)
 *
 * @param unit - unit to remove
 */
void ureg_remove_unit(Unit *unit);

/**
 * De-init and remove all units
 */
void ureg_remove_all_units(void);

/**
 * Save all units to a binary buffer
 * @param ctx
 */
void ureg_save_units(PayloadBuilder *pb);

/**
 * Load units from the binary format
 * @param ctx
 */
bool ureg_load_units(PayloadParser *pp);

/**
 * Export unit as INI to a buffer.
 *
 * @param index - unit index
 * @param iw - iniwriter instance to use
 * @return real number of bytes used (should end with a newline)
 */
void ureg_export_unit(uint32_t index, IniWriter *iw);

/**
 * Export everything to INI
 *
 * @param iw
 */
void ureg_build_ini(IniWriter *iw);

/**
* Get number of instantiated units
*
* @return nr of units
*/
uint32_t ureg_get_num_units(void);

/**
 * Instantiate a unit by INI
 *
 * This is called for lines inside the [UNITS] section, e.g. PIN=LED1, BUTTON
 *
 * @param driver_name - unit type ID
 * @param names - names string, comma separated (may have whitespace after commas)
 * @return all OK
 */
bool ureg_instantiate_by_ini(const char *restrict driver_name, const char *restrict names);

/**
 * Load a single INI line to a unit.
 *
 * @param name - unit name (for look-up)
 * @param key - property key
 * @param value - value to set as string
 * @param callsign - callsign (is part of the section string)
 * @return success
 */
bool ureg_load_unit_ini_key(const char *restrict name,
                            const char *restrict key,
                            const char *restrict value,
                            uint8_t callsign);

/**
 * Run init() for all unit instances.
 *
 * @return all OK
 */
bool ureg_finalize_all_init(void);

/**
 * Deliver a TinyFrame message to it's designed destination.
 * Unit is identified by the data first byte which is the "call sign"
 *
 * @param msg - message to deliver
 * @return true if delivered
 */
void ureg_deliver_unit_request(TF_Msg *msg);

/**
 * Report all unit callsigns and names to TF master
 *
 * @param frame_id - original message ID
 */
void ureg_report_active_units(TF_ID frame_id);

#endif //GEX_UNIT_REGISTRY_H
