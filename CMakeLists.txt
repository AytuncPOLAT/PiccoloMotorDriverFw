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
set(CMAKE_PROJECT_NAME PiccoloMotorDriver)

# Select the PCB-specific firmware profile. PiccoloZero is intentionally
# unavailable until its pinout and electrical values are defined.
set(BOARD "Piccolo2" CACHE STRING "PCB profile: Piccolo2 or PiccoloZero")
set_property(CACHE BOARD PROPERTY STRINGS Piccolo2 PiccoloZero)
if(BOARD STREQUAL "Piccolo2")
    set(BOARD_COMPILE_DEFINITION BOARD_PICCOLO2)
    set(BOARD_GATE_DRIVER "DRV8353S")
elseif(BOARD STREQUAL "PiccoloZero")
    message(FATAL_ERROR "PiccoloZero profile is incomplete; provide its pinout and electrical values")
else()
    message(FATAL_ERROR "Unsupported BOARD '${BOARD}'. Select Piccolo2 or PiccoloZero.")
endif()

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
    AppLayer/AnalogProcessor.cpp
    AppLayer/App.cpp
    AppLayer/Communication.cpp
    AppLayer/DebugPrint.cpp
    AppLayer/MotorControl.cpp
    AppLayer/PidControl.cpp
    AppLayer/PowerSupply.cpp
    AppLayer/SignalProcessing.cpp
    AppLayer/SimpleLogger.cpp
    AppLayer/SinusPwm.cpp
    AppLayer/SystemDataController.cpp
    AppLayer/SystemError.cpp
    AppLayer/Telemetry.cpp
    AppLayer/UserInterface.cpp
    AppLayer/Common/Crc16.cpp
    AppLayer/Common/SystemData.cpp
    HardwareLayer/AdcDriver.cpp
    HardwareLayer/AS5047_Driver.cpp
    HardwareLayer/SpiBus.cpp
    HardwareLayer/FlashMemoryController.cpp
    HardwareLayer/Hardware.cpp
    HardwareLayer/MotorPwm.cpp
    HardwareLayer/QuadraticEncoderDriver.cpp
    HardwareLayer/RS485.cpp
    HardwareLayer/FdcanDriver.cpp
    HardwareLayer/UsbVirtualCom.cpp
    Core/Src/freertos.c
    Core/Src/stm32h7xx_hal_timebase_tim.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    AppLayer
    HardwareLayer
    AppLayer/Common
    Core/Src
)

message("PCB profile: " ${BOARD})
message("Gate driver: " ${BOARD_GATE_DRIVER})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    HardwareLayer/${BOARD_GATE_DRIVER}_SpiDriver.cpp
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    ${BOARD_COMPILE_DEFINITION}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMENT "Generating Intel HEX image"
    VERBATIM
)
