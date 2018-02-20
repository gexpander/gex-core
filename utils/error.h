//
// Error codes and labels. Loosely based on DAPLink, with more codes added.
//

#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
}
#endif

// fields: name, msg. If msg is NULL, name as string will be used instead.
#define X_ERROR_CODES \
    /* Shared errors */  \
    X(SUCCESS, NULL) /* operation succeeded / unit loaded. Must be 0 */ \
    X(FAILURE, NULL) /* generic error. If returned from a unit handler, does NOT generate a response. */   \
    X(INTERNAL_ERROR, NULL) /* a bug */    \
    X(LOADING, NULL) /* unit is loading */ \
    X(UNKNOWN_COMMAND, NULL) \
    X(NOT_IMPLEMENTED, NULL) \
    X(NO_SUCH_UNIT, NULL) \
    X(BAD_UNIT_TYPE, NULL) \
    X(BAD_COUNT, NULL) \
    X(MALFORMED_COMMAND, NULL) \
    X(NOT_APPLICABLE, NULL) \
    X(HW_TIMEOUT, NULL)        /* timed out waiting for response, or received no ACK from hw device */ \
    X(BUS_FAULT, NULL)         /* further unspecified hardware bus fault */ \
    X(CHECKSUM_MISMATCH, NULL) /* bus checksum failed */ \
    X(PROTOCOL_BREACH, NULL)   /* eating with the wrong spoon */  \
    X(BUSY, NULL)              /* Unit is busy */ \
    X(BAD_MODE, NULL)        /* Command not permissible in current opmode */ \
    \
    /* VFS user errors (those are meant to be shown to user) */ \
    X(VFS_ERROR_DURING_TRANSFER, "Error during transfer") \
    X(VFS_TRANSFER_TIMEOUT, "Transfer timed out") \
    X(VFS_OOO_SECTOR, "File sent out of order by PC") \
    \
    /* File stream errors/status */ \
    X(VFS_SUCCESS_DONE, NULL) \
    X(VFS_SUCCESS_DONE_OR_MORE, NULL) \
    \
    /* Unit loading */ \
    X(BAD_CONFIG, "Configuration error") \
    X(BAD_KEY, "Unexpected config key") \
    X(BAD_VALUE, "Bad config value") \
    X(OUT_OF_MEM, "Not enough RAM") \
    X(RESOURCE_NOT_AVAILABLE, NULL)


/**
 * The return value for all functions with error reporting.
 */
typedef enum {
#define X(name, text) E_##name,
    X_ERROR_CODES
#undef X
    ERROR_COUNT
} error_t;


/** Check return value and return it if not E_SUCCESS */
#define TRY(call) do { \
    error_t _rv; \
    _rv = call; \
    if (E_SUCCESS != _rv) return _rv; \
} while (0)


/**
 * Get a user-friendly message from a E_* enum value
 *
 * @param error - E_* value
 * @return string, error name or description
 */
const char *error_get_message(error_t error) __attribute__((pure));


/**
 * Get error name from a E_* enum value
 *
 * @param error - E_* value
 * @return string, error name
 */
const char *error_get_name(error_t error) __attribute__((pure));

#ifdef __cplusplus
}
#endif

#endif
