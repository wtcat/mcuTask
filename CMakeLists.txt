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
set(WKSPACE_PATH  ${CMAKE_CURRENT_SOURCE_DIR})

# Import helper function
include(cmake/cpu.cmake)
include(cmake/utils.cmake)
include(cmake/zephyr.cmake)


# Declare project
project(mcuTask
    LANGUAGES C CXX ASM
)

#Import kconfig parser
include(cmake/kconfig.cmake)

# Set output path for library
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libs)

# Enable link time optimize
if (CONFIG_LTO)
  compile_options(-flto=auto)
  set(CMAKE_AR      ${CMAKE_C_COMPILER}-ar)
  set(CMAKE_RANLIB  ${CMAKE_C_COMPILER}-ranlib)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=auto -fuse-linker-plugin -specs=nano.specs")
endif(CONFIG_LTO)

# Parse optimize options
separate_arguments(COMPILER_OPT_AS_LIST UNIX_COMMAND ${CONFIG_COMPILER_OPT})

# Set common compile options
include_dirs(
    ${WKSPACE_PATH}
)

compile_options(
  -imacros ${AUTOCONF_H}
  -fdata-sections
  -ffunction-sections
  -Wall
  -Wextra
  -Werror 
  ${COMPILER_OPT_AS_LIST}
)

# Set linker script file
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/board/${BOARD}/${CONFIG_LINKER_SCRIPT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T ${LINKER_SCRIPT}")

#LLEXT options
if (CONFIG_LLEXT)
  include(cmake/llext_target.cmake)
endif()


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
