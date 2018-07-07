#####################################################
# Platform-independent, not directly related to GEX
#####################################################

# C
COMMON_C_DIRS = \
    Drivers \
    Drivers/CMSIS \
    Middlewares \
    Middlewares/FreeRTOS

COMMON_C_FILES =  \
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
    Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
    Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
    Middlewares/Third_Party/FreeRTOS/Source/list.c \
    Middlewares/Third_Party/FreeRTOS/Source/queue.c \
    Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
    Middlewares/Third_Party/FreeRTOS/Source/timers.c \
    Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
    Src/main.c

COMMON_C_FLAGS = \
    -DUSE_HAL_DRIVER \
    -DUSE_FULL_LL_DRIVER \
    -std=gnu99 -Wfatal-errors \
    -Wall -Wextra -Wshadow \
    -Wwrite-strings -Wold-style-definition -Winline -Wstrict-prototypes -Wreturn-type \
    -Wredundant-decls -Wfloat-equal -Wsign-compare \
    -fno-common -ffunction-sections -fdata-sections -Wno-unused-function \
    -MD -Wno-redundant-decls -Wno-unused-parameter \
    -Wno-unused-variable -Wno-inline \
    -fmerge-constants -fmerge-all-constants -Wno-implicit-fallthrough \
    -fno-exceptions -finline-small-functions -findirect-inlining -Wno-strict-aliasing -Wno-float-equal \
    -Wno-discarded-qualifiers -fstack-usage
    
ifeq ($(DEBUG), 1)
COMMON_C_FLAGS += -ggdb -g -gdwarf-2
endif

COMMON_C_INCLUDES = \
    -IInc \
    -IMiddlewares/Third_Party/FreeRTOS/Source/include \
    -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
    -IDrivers/CMSIS/Include

# ASM
COMMON_AS_DIRS = \

COMMON_AS_FILES = \

COMMON_AS_FLAGS = \
    -Wall -fdata-sections -ffunction-sections

COMMON_AS_INCLUDES = \
    -IInc

#####################################################
# Platform-independent, GEX specific
#####################################################

GEX_UNIT_DIRS = $(foreach x,$(GEX_UNITS), GexUnits/$(x))
GEX_UNIT_DEFS = $(foreach x,$(GEX_UNITS), -DENABLE_UNIT_$(shell echo $(x) | tr a-z A-Z)=1)
ENDPAREN = )
GEX_UNIT_DEFS += -DUNITS_REGISTER_CMD="$(foreach x,$(GEX_UNITS),ureg_add_type(&UNIT_$(shell echo $(x) | tr a-z A-Z));)"

GEX_C_DIRS = \
    GexCore \
    GexCore/utils \
    GexCore/comm \
    GexCore/framework \
    GexCore/platform \
    GexCore/units \
    GexCore/TinyFrame \
    GexCore/tasks \
    $(GEX_UNIT_DIRS)

GEX_C_FILES = \
    GexCore/USB/usb_device.c \
    GexCore/USB/usbd_cdc_if.c \
    GexCore/USB/usbd_conf.c \
    GexCore/USB/usbd_desc.c \
    GexCore/USB/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
    GexCore/USB/STM32_USB_Device_Library/Class/MSC_CDC/usbd_msc_cdc.c \
    GexCore/USB/STM32_USB_Device_Library/Core/Src/usbd_core.c \
    GexCore/USB/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
    GexCore/USB/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

GEX_C_INCLUDES = \
    -IGexUnits \
    -IGexCore \
    -IGexCore/USB \
    -IGexCore/USB/MSC_CDC \
    -IGexCore/TinyFrame \
    -IGexCore/vfs \
    -IGexCore/utils \
    -IGexCore/units \
    -IGexCore/framework \
    -IGexCore/platform \
    -IGexCore/tasks \
    -IGexCore/USB/STM32_USB_Device_Library/Core/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/AUDIO/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/CDC/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/CustomHID/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/DFU/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/HID/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/MSC/Inc \
    -IGexCore/USB/STM32_USB_Device_Library/Class/MSC_CDC

GEX_C_FLAGS = \
    -DGEX_PLAT_$(GEX_PLAT) \
    -D__weak="__attribute__((weak))" \
    -D__packed="__attribute__((__packed__))" \
    $(GEX_UNIT_DEFS)

# Option to disable debug
ifeq '$(DISABLE_DEBUG)' '1'
GEX_C_FLAGS += \
    -DUSE_FULL_ASSERT=0 \
    -DASSERT_FILENAMES=0 \
    -DDEBUG_VFS=0 \
    -DDEBUG_FLASH_WRITE=0 \
    -DVERBOSE_HARDFAULT=0 \
    -DUSE_STACK_MONITOR=0 \
    -DUSE_DEBUG_UART=0 \
    -DDEBUG_MALLOC=0 \
    -DDEBUG_RSC=0
else
GEX_C_FLAGS += \
    -DUSE_FULL_ASSERT=1 \
    -DASSERT_FILENAMES=1 \
    -DDEBUG_VFS=0 \
    -DDEBUG_FLASH_WRITE=0 \
    -DVERBOSE_HARDFAULT=1 \
    -DUSE_STACK_MONITOR=0 \
    -DUSE_DEBUG_UART=1 \
    -DDEBUG_MALLOC=0 \
    -DDEBUG_RSC=0
endif


# Option to disable mass storage
ifeq '$(DISABLE_MSC)' '1'
GEX_C_FLAGS += -DDISABLE_MSC
else
GEX_C_FILES += \
    GexCore/USB/usbd_storage_if.c \
    GexCore/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc.c \
    GexCore/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_bot.c \
    GexCore/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_data.c \
    GexCore/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_scsi.c
GEX_C_DIRS += \
    GexCore/vfs
endif


# Option to enable CDC loopback (testing USB)
ifeq '$(CDC_LOOPBACK_TEST)' '1'
GEX_C_FLAGS += -DCDC_LOOPBACK_TEST=1
endif


#####################################################
# Common build infrastructure
#####################################################

# Build path
BUILD_DIR = build
# target
TARGET = firmware
# Specs (determines which stdlib version is included)
SPECS = nano.specs

C_SOURCES = \
    $(COMMON_C_FILES) $(PLAT_C_FILES) $(GEX_C_FILES) \
    $(foreach x,$(COMMON_C_DIRS),$(wildcard $(x)/*.c)) \
    $(foreach x,$(PLAT_C_DIRS),$(wildcard $(x)/*.c)) \
    $(foreach x,$(GEX_C_DIRS),$(wildcard $(x)/*.c)) \

AS_SOURCES = \
    $(COMMON_AS_FILES) $(PLAT_AS_FILES) $(GEX_AS_FILES) 
    $(foreach x,$(COMMON_AS_DIRS),$(wildcard $(x)/*.s)) \
    $(foreach x,$(PLAT_AS_DIRS),$(wildcard $(x)/*.s)) \
    $(foreach x,$(GEX_AS_DIRS),$(wildcard $(x)/*.s))

# mcu
MCU = $(PLAT_CPU) -mthumb $(PLAT_FPU) $(PLAT_FLOAT-ABI)

#######################################
# binaries
#######################################
BINPATH = /usr/bin
PREFIX = arm-none-eabi-
CC = $(BINPATH)/$(PREFIX)gcc
AS = $(BINPATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(BINPATH)/$(PREFIX)objcopy
AR = $(BINPATH)/$(PREFIX)ar
SZ = $(BINPATH)/$(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S


LIBS = -lc -lm -lnosys
LDFLAGS = \
    $(MCU) -specs=$(SPECS) -T$(PLAT_LDSCRIPT) $(LIBS) \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
    -Wl,--gc-sections

# optimization (possible to set in build.mk)
ifeq ($(OPT),)
OPT = -Os
endif

C_FLAGS = \
    $(MCU) $(OPT) \
    $(GEX_C_INCLUDES) $(PLAT_C_INCLUDES) $(COMMON_C_INCLUDES) \
    $(GEX_C_FLAGS) $(PLAT_C_FLAGS) $(COMMON_C_FLAGS)

AS_FLAGS = \
    $(MCU) $(OPT) \
    $(GEX_AS_INCLUDES) $(PLAT_AS_INCLUDES) $(COMMON_AS_INCLUDES) \
    $(GEX_AS_FLAGS) $(PLAT_AS_FLAGS) $(COMMON_AS_FLAGS)


# Generate dependency information
C_FLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"


#######################################
# Targets
#######################################

# # list of objects
# OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
# vpath %.c $(sort $(dir $(C_SOURCES)))
# # list of ASM program objects
# OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(AS_SOURCES:.s=.o)))
# vpath %.s $(sort $(dir $(AS_SOURCES)))

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(AS_SOURCES:.s=.o))
vpath %.s $(sort $(dir $(AS_SOURCES)))

MAKEFILES = Makefile GexCore/gex.mk build.mk

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c $(MAKEFILES) | $(BUILD_DIR)
	@echo -e "\x1b[32mCC\x1b[m $<\n  \x1b[90m-> $@\x1b[m"
	@mkdir -p `dirname "$(BUILD_DIR)/$(<)"`
	@$(CC) -c $(C_FLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(<:.c=.lst) $< -o $@

$(BUILD_DIR)/%.o: %.s $(MAKEFILES) | $(BUILD_DIR)
	@$(AS) -c $(C_FLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(MAKEFILES)
	@printf "LD $< -> $@\n"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf $(MAKEFILES) | $(BUILD_DIR)
	@printf "OBJDUMP $< -> $(BUILD_DIR)/$(TARGET).hex \n"
	@$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf $(MAKEFILES) | $(BUILD_DIR)
	@printf "OBJDUMP $< -> $(BUILD_DIR)/$(TARGET).bin \n"
	@$(BIN) $< $@

flash: $(BUILD_DIR)/$(TARGET).bin $(MAKEFILES)
	@printf "FLASH  $<\n"
	@st-flash write $< 0x8000000

$(BUILD_DIR)/$(TARGET).dfu: $(BUILD_DIR)/$(TARGET).hex $(MAKEFILES)
	@printf "DFU GEN  $<\n"
	dfu-convert -i $< $@

dfu: $(BUILD_DIR)/$(TARGET).dfu $(MAKEFILES)
	@printf "DFU UPLOAD  $<\n"
	dfu-util -a 0 -D $<

dis: $(BUILD_DIR)/$(TARGET).elf $(MAKEFILES)
	@printf "DIS $<\n"
	@arm-none-eabi-objdump -Sslrtd build/$(TARGET).elf > disassembly.lst

$(BUILD_DIR):
	@mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
