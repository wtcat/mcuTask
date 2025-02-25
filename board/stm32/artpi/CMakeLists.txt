cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# Setup toolchain
set(CROSS_COMPILER arm-none-eabi-)
set(WKSAPCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${WKSAPCE_PATH}/cmake/cortex_m7.cmake)
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
# set(CONFIG_NETX   1)
# set(CONFIG_FILEX  1)
# set(CONFIG_USBX   1)
# set(CONFIG_LEVELX 1)
set(CONFIG_KMALLOC 1)
set(CONFIG_MALLOC  1)
set(CONFIG_SUBSYS_CLI 1)
set(CONFIG_SUBSYS_SD  1)

# Add configure files
set(TX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)

# Add compile options
include_dirs(
    ${WKSAPCE_PATH}
    ${WKSAPCE_PATH}/basework/arch/arm
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_compile_options(
    -DSTM32H750xx=1
    -DUSE_HAL_DRIVER=1
    -DUSE_FULL_LL_DRIVER=1
    -mlong-calls
    -Wno-unused-parameter
)

add_executable(${PROJECT_NAME}
    system_stm32h7xx.c
    main.c
    stm32_startup.c
    stm32_uart.c
    stm32_pins.c
    stm32_gpio.c
)

# Add subdirectory
add_subdirectory(${WKSAPCE_PATH}/rtos rtos)
add_subdirectory(${WKSAPCE_PATH}/basework basework)
add_subdirectory(${WKSAPCE_PATH}/subsys subsys)
add_subdirectory(${WKSAPCE_PATH}/board/shared shared)
add_subdirectory(${WKSAPCE_PATH}/board/stm32/shared stm32_shared)
add_subdirectory(${WKSAPCE_PATH}/board/stm32/shared/h7/Drivers drivers)

# Link target
collect_link_libraries(libs ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
    -Wl,--whole-archive
    -Wl,--start-group
    ${libs}
    -Wl,--end-group
    -Wl,--no-whole-archive
)

# Generate binary
build_post(${PROJECT_NAME})
