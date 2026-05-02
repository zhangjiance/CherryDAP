cmake_minimum_required(VERSION 3.13)

# Toolchain settings - MUST be set before project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Project name
project(apm32e103_cherrydap C ASM)

# System clock selection (MHz): 72, 96, 120 (default)
set(APM32_SYSCLK_MHZ "120" CACHE STRING "APM32 SYSCLK in MHz (72/96/120)")
set_property(CACHE APM32_SYSCLK_MHZ PROPERTY STRINGS 72 96 120)

if(APM32_SYSCLK_MHZ STREQUAL "72")
    set(APM32_SYSCLK_DEFINE CHERRYDAP_SYSCLK_72M)
    set(APM32_SYSCLK_SUFFIX 72m)
elseif(APM32_SYSCLK_MHZ STREQUAL "96")
    set(APM32_SYSCLK_DEFINE CHERRYDAP_SYSCLK_96M)
    set(APM32_SYSCLK_SUFFIX 96m)
elseif(APM32_SYSCLK_MHZ STREQUAL "120")
    set(APM32_SYSCLK_DEFINE CHERRYDAP_SYSCLK_120M)
    set(APM32_SYSCLK_SUFFIX 120m)
else()
    message(FATAL_ERROR "Unsupported APM32_SYSCLK_MHZ='${APM32_SYSCLK_MHZ}'. Use one of: 72, 96, 120")
endif()

message(STATUS "APM32 SYSCLK: ${APM32_SYSCLK_MHZ} MHz")

set(APM32_ARTIFACT_NAME "${PROJECT_NAME}_${APM32_SYSCLK_SUFFIX}")

# MCU settings
set(MCU_FAMILY APM32E10x)
set(MCU_MODEL APM32E103xE)
set(MCU_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/apm32e103_app.ld)

# CPU and compiler options
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -Wall -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O3 -flto")

# ASM flags
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T${MCU_LINKER_SCRIPT} \
-Wl,-Map=${APM32_ARTIFACT_NAME}.map,--cref -Wl,--gc-sections \
-specs=nano.specs -specs=nosys.specs \
-Wl,--print-memory-usage")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")

# Macro definitions
add_definitions(
    -DAPM32E10X_HD
    -DUSE_STDPERIPH_DRIVER
    -D${APM32_SYSCLK_DEFINE}
    -DVECT_TAB_OFFSET=0x2000  # Application starts at 0x08002000
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Device/Geehy/APM32E10x/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/cdc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/dfu
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/hid
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/msc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Source files - Device startup and system
set(DEVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Device/Geehy/APM32E10x/Source/gcc/startup_apm32e10x_hd.S
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Device/Geehy/APM32E10x/Source/system_apm32e10x.c
)

# Source files - APM32E10x_StdPeriphDriver (add only needed peripherals)
file(GLOB STDPERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_rcm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_dma.c
)

# CherryUSB sources
set(CHERRYUSB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/class/cdc/usbd_cdc_acm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/port/fsdev/usb_dc_fsdev.c
)

# CherryRB sources
set(CHERRYRB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryRB/chry_ringbuffer.c
)

# DAP sources
set(DAP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dap_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/DAP_vendor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/JTAG_DP.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../DAP/Source/SW_DP.c
)

# Application sources
file(GLOB APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c
)

# All sources
set(SOURCES
    ${DEVICE_SOURCES}
    ${STDPERIPH_SOURCES}
    ${CHERRYUSB_SOURCES}
    ${CHERRYRB_SOURCES}
    ${DAP_SOURCES}
    ${APP_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${APM32_ARTIFACT_NAME})

# Create hex file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${APM32_ARTIFACT_NAME}.hex
    COMMENT "Creating HEX file"
)

# Create bin file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${APM32_ARTIFACT_NAME}.bin
    COMMENT "Creating BIN file"
)

# Print size
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size of firmware"
)
