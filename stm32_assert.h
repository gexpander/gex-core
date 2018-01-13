//
// Created by MightyPork on 2017/11/20.
//

#ifndef STM32_ASSERT_H
#define STM32_ASSERT_H

#include <stdint.h>

void __attribute__((noreturn)) abort_msg(const char *msg, const char *filename, uint32_t line);
void warn_msg(const char *msg, const char *filename, uint32_t line);
void __attribute__((noreturn)) assert_failed_(const char *file, uint32_t line);

#if USE_FULL_ASSERT
    #if ASSERT_FILENAMES
        // With the filename enabled.
        #define trap(msg) abort_msg(msg, __BASE_FILE__, __LINE__)
        #define assert_param(expression) do { if (!(expression)) assert_failed_(__BASE_FILE__, __LINE__); } while(0)
        #define assert_warn(expression, msg) do { if (!(expression)) warn_msg(msg, __BASE_FILE__, __LINE__); } while(0)
        #define _Error_Handler(file, line) assert_failed_(__BASE_FILE__, __LINE__)
    #else
        // Filename disabled to save code size.
        #define trap(msg) abort_msg(msg, "??", __LINE__)
        #define assert_param(expression) do { if (!(expression)) assert_failed_("??", __LINE__); } while(0)
        #define assert_warn(expression, msg) do { if (!(expression)) warn_msg(msg, "??", __LINE__); } while(0)
        #define _Error_Handler(file, line) assert_failed_("??", __LINE__)
    #endif
#else
    // This is after everything is well tested, to cut some flash and make code faster by removing checks
    #define trap(msg) do {} while(1)
    #define assert_param(expression) do { (void)(expression); } while(0)
    #define assert_warn(expression, msg) do { (void)(expression); } while(0)
    #define _Error_Handler(file, line) do {} while(1)
#endif

#endif //STM32_ASSERT_H
