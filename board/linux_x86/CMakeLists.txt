cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# Setup toolchain
set(WKSAPCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(THREADX_ARCH "linux")
set(THREADX_TOOLCHAIN "gnu")
include(${WKSAPCE_PATH}/cmake/utils.cmake)
include(${WKSAPCE_PATH}/cmake/zephyr.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# Declare project
project(mcutask
    LANGUAGES C CXX ASM
)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif(POLICY CMP0076)

# Configure options
include(${WKSAPCE_PATH}/cmake/options.cmake)
# set(CONFIG_NETX   1)
set(CONFIG_FILEX  1)
# set(CONFIG_USBX   1)
# set(CONFIG_LEVELX 1)
# set(CONFIG_KMALLOC 1)
# set(CONFIG_SUBSYS_CLI 1)
# set(CONFIG_SUBSYS_SD  1)
set(CONFIG_SUBSYS_FS 1)
set(CONFIG_CJSON 1)

# Add configure files
set(TX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)

if (CONFIG_FILEX)
    set(FILEX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/port)
    set(FX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/fx_user.h)
endif()
if (CONFIG_NETX)
    set(NETXDUO_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/)
    set(NX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/nx_user.h)
endif()
if (CONFIG_USBX)
    set(USBX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/)
    set(UX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/ux_user.h)
endif()

# Add compile options
include_dirs(
    ${WKSAPCE_PATH}
    ${WKSAPCE_PATH}/basework/arch/x86
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -fno-exceptions")

add_compile_options(
    -DCONFIG_C11_MEM_MODEL=1
    -DTX_LINUX_DEBUG_ENABLE

    -Wall
    -Wextra
    -fdata-sections 
    -ffunction-sections
    -Wno-unused-parameter
    -Wno-implicit-fallthrough
    -O0 
    -g
)


set (BOARD_SOURCES
    main.c
)

add_executable(${PROJECT_NAME}
    ${BOARD_SOURCES}
)

# Add subdirectory
add_subdirectory(${WKSAPCE_PATH}/rtos rtos)
add_subdirectory(${WKSAPCE_PATH}/basework basework)
add_subdirectory(${WKSAPCE_PATH}/subsys subsys)
add_subdirectory(${WKSAPCE_PATH}/board/shared shared)

# Linker options
target_link_options(${PROJECT_NAME}
    PRIVATE
    -Wl,-Map=${CMAKE_BINARY_DIR}/mcutask.map
    -Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

# Link target
collect_link_libraries(libs ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
    -Wl,--whole-archive
    -Wl,--start-group
    ${libs}
    pthread
    rt
    -Wl,--end-group
    -Wl,--no-whole-archive
)
