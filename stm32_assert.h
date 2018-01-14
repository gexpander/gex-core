//
// Created by MightyPork on 2017/11/20.
//

#ifndef STM32_ASSERT_H
#define STM32_ASSERT_H

#include <stdint.h>
#include "debug.h"

void _abort_errlight(void);
void __attribute__((noreturn)) _assert_failed(const char *file, uint32_t line);
void __attribute__((noreturn)) _abort_do(void);

#define _abort_msg(file, line, format, ...) do { \
    _abort_errlight(); \
    dbg("\r\n\033[31m" format ":\033[m %s:%d", ##__VA_ARGS__, file, (int)line); \
    _abort_do(); \
} while (0)

#define _warn_msg(file, line, format, ...) do { \
    dbg("\r\n\033[33m" format ":\033[m %s:%d", ##__VA_ARGS__, file, (int)line); \
} while (0)

#if USE_FULL_ASSERT
    #if ASSERT_FILENAMES
        // With the filename enabled.
        #define trap(msg, ...) _abort_msg(__BASE_FILE__, __LINE__, msg, ##__VA_ARGS__)
        #define warn(msg, ...) _warn_msg(__BASE_FILE__, __LINE__, msg, ##__VA_ARGS__)
        #define assert_param(expression) do { if (!(expression)) _assert_failed(__BASE_FILE__, __LINE__); } while(0)
        #define assert_warn(expression, msg, ...) do { if (!(expression)) _warn_msg(__BASE_FILE__, __LINE__, msg, ##__VA_ARGS__); } while(0)
        #define _Error_Handler(file, line) _assert_failed(__BASE_FILE__, __LINE__)
    #else
        // Filename disabled to save code size.
        #define trap(msg, ...) _abort_msg("??", __LINE__, msg, ##__VA_ARGS__)
        #define warn(msg, ...) _warn_msg("??", __LINE__, msg, ##__VA_ARGS__)
        #define assert_param(expression) do { if (!(expression)) _assert_failed("??", __LINE__); } while(0)
        #define assert_warn(expression, msg, ...) do { if (!(expression)) _warn_msg("??", __LINE__, msg, ##__VA_ARGS__); } while(0)
        #define _Error_Handler(file, line) _assert_failed("??", __LINE__)
    #endif
#else
    // This is after everything is well tested, to cut some flash and make code faster by removing checks
    // must take care to evaluate the expressions regardless in case they have side effects.
    #define trap(msg, ...) do {} while(1)
    #define warn(msg, ...)
    #define assert_param(expression) do { (void)(expression); } while(0)
    #define assert_warn(expression, msg, ...) do { (void)(expression); } while(0)
    #define _Error_Handler(file, line) do {} while(1)
#endif

#endif //STM32_ASSERT_H
