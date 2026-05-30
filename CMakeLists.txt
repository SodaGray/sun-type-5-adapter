cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME sun-type-5-adapter)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# ───────────────────────── TinyUSB sources ─────────────────────────
set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/extern/tinyusb/src)

set(TINYUSB_SOURCES
        # Core
        ${TINYUSB_DIR}/tusb.c
        ${TINYUSB_DIR}/common/tusb_fifo.c

        # Device stack
        ${TINYUSB_DIR}/device/usbd.c
        ${TINYUSB_DIR}/device/usbd_control.c

        # HID class
        ${TINYUSB_DIR}/class/hid/hid_device.c

        # DCD for STM32 OTG FS (Synopsys DWC2)
        ${TINYUSB_DIR}/portable/synopsys/dwc2/dcd_dwc2.c
        ${TINYUSB_DIR}/portable/synopsys/dwc2/dwc2_common.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${TINYUSB_SOURCES}
        App/usb_descriptors.c
        App/ring_buf.c
        App/sun_io.c
        App/sun_protocol.c
        App/sun_keymap.c
        App/hid_keyboard.c
        App/w25q.c
        App/crc32.c
        App/sector_io.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined include paths
        ${CMAKE_SOURCE_DIR}/App
        ${CMAKE_SOURCE_DIR}/extern/tinyusb/src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined symbols
        CFG_TUSB_MCU=OPT_MCU_STM32L4
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
