############################################################
# Copyright 2025 wtcat(wt1454246140@gmail.com)
# McuTask cmake project
############################################################
cmake_minimum_required(VERSION 3.0.0...3.8.0 FATAL_ERROR)

# Set default toolchain
set(CROSS_COMPILER arm-none-eabi-)

# Set board information
if (NOT DEFINED BOARD OR NOT DEFINED THREADX_ARCH)
    include(board_cfg.cmake)
endif()

# Set workspace path
set(WKSAPCE_PATH  ${CMAKE_CURRENT_SOURCE_DIR})

# Import helper function
include(cmake/cpu.cmake)
include(cmake/utils.cmake)
include(cmake/zephyr.cmake)

# Generate compile database
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# Declare project
project(mcuTask
    LANGUAGES C CXX ASM
)

#Import kconfig parser
include(cmake/kconfig.cmake)

cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0003 NEW)

# Add execute target
add_executable(${PROJECT_NAME} "")

add_subdirectory(board)
add_subdirectory(application)
add_subdirectory(rtos)
add_subdirectory(base)
add_subdirectory(subsys)

# Link target
build_link(${PROJECT_NAME})

# Generate binary
build_post(${PROJECT_NAME})
