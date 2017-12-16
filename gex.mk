GEX_SRC_DIR = \
User \
User/utils \
User/USB \
User/comm \
User/framework \
User/platform \
User/units \
User/units/system \
User/units/neopixel \
User/units/pin \
User/TinyFrame \
User/CWPack \
User/USB/MSC_CDC \
User/vfs

GEX_INCLUDES = \
-IUser \
-IUser/USB \
-IUser/USB/MSC_CDC \
-IUser/TinyFrame \
-IUser/vfs \
-IUser/utils \
-IUser/units \
-IUser/units/system \
-IUser/units/neopixel \
-IUser/units/pin \
-IUser/framework \
-IUser/platform

GEX_CFLAGS      = -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))"
GEX_CFLAGS      += -std=gnu99 -Wfatal-errors
GEX_CFLAGS      += -Wall -Wextra -Wshadow
GEX_CFLAGS      += -Wwrite-strings -Wold-style-definition -Winline -Wno-missing-noreturn -Wstrict-prototypes -Wreturn-type
GEX_CFLAGS      += -Wredundant-decls -Wfloat-equal -Wsign-compare
GEX_CFLAGS      += -fno-common -ffunction-sections -fdata-sections -Wno-unused-function
GEX_CFLAGS      += -MD -Wno-format-zero-length -Wno-redundant-decls -Wno-unused-parameter
GEX_CFLAGS      += -Wno-discarded-qualifiers -Wno-unused-variable -Wno-inline
GEX_CFLAGS      += -Wno-float-equal -Wno-implicit-fallthrough -Wno-strict-aliasing
GEX_CFLAGS      += -fmerge-constants -fmerge-all-constants
GEX_CFLAGS      += -fno-exceptions -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -finline-small-functions -findirect-inlining

GEX_CDEFS = \
-D__weak="__attribute__((weak))" \
-D__packed="__attribute__((__packed__))" \
-DUSE_FULL_LL_DRIVER \
-DUSE_FULL_ASSERT=1 \
-DVERBOSE_ASSERT=1 \
-DDEBUG_VFS=1 \
-DVERBOSE_HARDFAULT=1 \
-DUSE_STACK_MONITOR=1 \
-DUSE_DEBUG_UART=1
