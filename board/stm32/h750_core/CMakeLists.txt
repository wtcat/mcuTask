cmake_minimum_required(VERSION 3.0.0...3.8.0 FATAL_ERROR)


# Setup toolchain
set(CROSS_COMPILER arm-none-eabi-)
set(WKSAPCE_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(BOARD          "stm32/h750_core")
include(${WKSAPCE_PATH}/cmake/cortex_m7.cmake)
include(${WKSAPCE_PATH}/cmake/utils.cmake)
include(${WKSAPCE_PATH}/cmake/zephyr.cmake)

# Declare project
project(mcutask
    LANGUAGES C CXX ASM
)

include(${WKSAPCE_PATH}/cmake/kconfig.cmake)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0003 NEW)

# Add configure files
set(TX_USER_FILE        ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)
set(FILEX_CUSTOM_PORT   ${WKSAPCE_PATH}/board/shared/port)
set(NETXDUO_CUSTOM_PORT ${WKSAPCE_PATH}/board/shared/port)
set(USBX_CUSTOM_PORT    ${WKSAPCE_PATH}/board/shared/port)


# Add compile options
include_dirs(
    ${WKSAPCE_PATH}
    ${WKSAPCE_PATH}/basework/arch/arm
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_compile_options(
    -imacros ${AUTOCONF_H}
    ${CONFIG_COMPILER_OPTIMIZE}
    -DSTM32H750xx=1
    -DUSE_HAL_DRIVER=1
    -DUSE_FULL_LL_DRIVER=1
    # -mlong-calls
    -Wno-unused-parameter
    -Wno-implicit-fallthrough
)

set (BOARD_SOURCES
    system_stm32h7xx.c
    stm32_startup.c
    stm32_qflash.c
    stm32_dma.c
    stm32_uart.c
    stm32_pins.c
    stm32_gpio.c
    stm32_leds.c
    stm32_gpiokey.c
    stm32_hrtimer.c
    stm32_i2c.c
    main.c
)

if (CONFIG_SUBSYS_SDIO)
    list(APPEND BOARD_SOURCES 
        stm32_sdmmc.c
    )
endif()

if (CONFIG_USBX)
    list(APPEND BOARD_SOURCES 
        # stm32_usbhost.c
        stm32_usbdevice.c
        stm32_usbmsc.c
    )
endif()

add_executable(${PROJECT_NAME}
    ${BOARD_SOURCES}
)

# Add subdirectory
add_subdirectory(${WKSAPCE_PATH}/application application)
add_subdirectory(${WKSAPCE_PATH}/rtos rtos)
add_subdirectory(${WKSAPCE_PATH}/base base)
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
