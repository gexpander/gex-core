GEX_SRC_DIR = \
    User \
    User/utils \
    User/comm \
    User/framework \
    User/platform \
    User/units \
    User/units/system \
    User/units/neopixel \
    User/units/pin \
    User/TinyFrame \
    User/CWPack \
    User/vfs \
    User/tasks

GEX_SOURCES = \
    User/USB/usb_device.c \
    User/USB/usbd_cdc_if.c \
    User/USB/usbd_conf.c \
    User/USB/usbd_desc.c \
    User/USB/usbd_storage_if.c \
    User/USB/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
    User/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc.c \
    User/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_bot.c \
    User/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_data.c \
    User/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_scsi.c \
    User/USB/STM32_USB_Device_Library/Class/MSC_CDC/usbd_msc_cdc.c \
    User/USB/STM32_USB_Device_Library/Core/Src/usbd_core.c \
    User/USB/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
    User/USB/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

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
    -IUser/platform \
    -IUser/tasks \
    -IUser/USB/STM32_USB_Device_Library/Core/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/AUDIO/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/CDC/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/CustomHID/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/DFU/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/HID/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/MSC/Inc \
    -IUser/USB/STM32_USB_Device_Library/Class/MSC_CDC

GEX_CFLAGS = \
    -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))"  \
    -std=gnu99 -Wfatal-errors \
    -Wall -Wextra -Wshadow \
    -Wwrite-strings -Wold-style-definition -Winline -Wno-missing-noreturn -Wstrict-prototypes -Wreturn-type \
    -Wredundant-decls -Wfloat-equal -Wsign-compare \
    -fno-common -ffunction-sections -fdata-sections -Wno-unused-function \
    -MD -Wno-format-zero-length -Wno-redundant-decls -Wno-unused-parameter \
    -Wno-discarded-qualifiers -Wno-unused-variable -Wno-inline \
    -Wno-float-equal -Wno-implicit-fallthrough -Wno-strict-aliasing \
    -fmerge-constants -fmerge-all-constants \
    -fno-exceptions -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -finline-small-functions -findirect-inlining \

GEX_CDEFS = \
    -D__weak="__attribute__((weak))" \
    -D__packed="__attribute__((__packed__))" \
    -DUSE_FULL_LL_DRIVER \
    -DUSE_FULL_ASSERT=1 \
    -DVERBOSE_ASSERT=1 \
    -DDEBUG_VFS=0 \
    -DDEBUG_FLASH_WRITE=0 \
    -DVERBOSE_HARDFAULT=1 \
    -DUSE_STACK_MONITOR=1 \
    -DUSE_DEBUG_UART=1
