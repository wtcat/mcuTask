
# Add configure files
cmake_setvar(TX_USER_FILE        ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)
cmake_setvar(FILEX_CUSTOM_PORT   ${WKSAPCE_PATH}/board/shared/port)
cmake_setvar(NETXDUO_CUSTOM_PORT ${WKSAPCE_PATH}/board/shared/port)
cmake_setvar(USBX_CUSTOM_PORT    ${WKSAPCE_PATH}/board/shared/port)


# Add compile options
include_dirs(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WKSAPCE_PATH}/board/shared/port
)
compile_options(
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
    # stm32_uart.c
    stm32_serial.c
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

target_sources(${PROJECT_NAME}
    PRIVATE
    ${BOARD_SOURCES}
)

add_subdirectory(${WKSAPCE_PATH}/board/stm32/shared stm32_shared)
add_subdirectory(${WKSAPCE_PATH}/board/stm32/shared/h7/Drivers drivers)
