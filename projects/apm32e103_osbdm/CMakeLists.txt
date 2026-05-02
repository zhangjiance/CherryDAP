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
project(apm32e103_osbdm C ASM)

# MCU settings
set(MCU_FAMILY APM32E10x)
set(MCU_MODEL APM32E103xE)
set(MCU_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/apm32e103_app.ld)

# CPU and compiler options
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -Wall -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O3 -flto")

# ASM flags
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T${MCU_LINKER_SCRIPT} \
-Wl,-Map=${PROJECT_NAME}.map,--cref -Wl,--gc-sections \
-specs=nano.specs -specs=nosys.specs \
-Wl,--print-memory-usage")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")

# Macro definitions
add_definitions(
    -DAPM32E10X_HD
    -DUSE_STDPERIPH_DRIVER
    -DVECT_TAB_OFFSET=0x2000  # Application starts at 0x08002000
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/osbdm
    ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/Device/Geehy/APM32E10x/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core
)

# Source files - Device startup and system
set(DEVICE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/Device/Geehy/APM32E10x/Source/gcc/startup_apm32e10x_hd.S
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/Device/Geehy/APM32E10x/Source/system_apm32e10x.c
)

# Source files - APM32E10x_StdPeriphDriver
file(GLOB STDPERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_rcm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../apm32e103/Drivers/APM32E10x_StdPeriphDriver/src/apm32e10x_misc.c
)

# CherryUSB sources
set(CHERRYUSB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/core/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../CherryUSB/port/fsdev/usb_dc_fsdev.c
)

# OSBDM core sources
set(OSBDM_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/cmd_processing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/board_id.c
)

# OSBDM JTAG driver selection (single active driver per build)
set(OSBDM_ACTIVE_JTAG_DRIVER "eppc" CACHE STRING "Active JTAG driver: dsc|eppc|kinetis|jm60")
set_property(CACHE OSBDM_ACTIVE_JTAG_DRIVER PROPERTY STRINGS dsc eppc kinetis jm60)

if(OSBDM_ACTIVE_JTAG_DRIVER STREQUAL "dsc")
    set(OSBDM_JTAG_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/jtag_dsc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/jtag_jm60.c
    )
elseif(OSBDM_ACTIVE_JTAG_DRIVER STREQUAL "eppc")
    set(OSBDM_JTAG_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/jtag_eppc.c
    )
elseif(OSBDM_ACTIVE_JTAG_DRIVER STREQUAL "kinetis")
    set(OSBDM_JTAG_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/jtag_kinetis.c
    )
elseif(OSBDM_ACTIVE_JTAG_DRIVER STREQUAL "jm60")
    set(OSBDM_JTAG_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/jtag_jm60.c
    )
else()
    message(FATAL_ERROR "Unsupported OSBDM_ACTIVE_JTAG_DRIVER='${OSBDM_ACTIVE_JTAG_DRIVER}'. Use one of: dsc, eppc, kinetis, jm60")
endif()

message(STATUS "OSBDM active JTAG driver: ${OSBDM_ACTIVE_JTAG_DRIVER}")

# Artifact name includes active target architecture/driver.
set(OSBDM_ARTIFACT_NAME "${PROJECT_NAME}_${OSBDM_ACTIVE_JTAG_DRIVER}")

# OSBDM BDM drivers
# Note: BDM drivers disabled for now. Need similar porting work as JTAG drivers.
set(OSBDM_BDM_SOURCES
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_cf.c          # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_cfv1.c        # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_9S08.c        # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_9S12.c        # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_9s12z.c       # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_9RS08.c       # TODO: needs GPIO and type fixes
    # ${CMAKE_CURRENT_SOURCE_DIR}/osbdm/bdm_bgnd_driver.c # TODO: needs GPIO and type fixes
)

# Application sources
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/board_osbdm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/usb_osbdm.c
)

# All sources
set(SOURCES
    ${DEVICE_SOURCES}
    ${STDPERIPH_SOURCES}
    ${CHERRYUSB_SOURCES}
    ${OSBDM_CORE_SOURCES}
    ${OSBDM_JTAG_SOURCES}
    ${OSBDM_BDM_SOURCES}
    ${APP_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${OSBDM_ARTIFACT_NAME})

# Create hex file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${OSBDM_ARTIFACT_NAME}.hex
    COMMENT "Creating HEX file"
)

# Create bin file
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${OSBDM_ARTIFACT_NAME}.bin
    COMMENT "Creating BIN file"
)

# Print size
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size of firmware"
)
