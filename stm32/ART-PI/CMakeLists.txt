cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# Setup toolchain
set(CROSS_COMPILER arm-none-eabi-)
set(WKSAPCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(${WKSAPCE_PATH}/cmake/cortex_m7.cmake)
include(${WKSAPCE_PATH}/cmake/utils.cmake)
include(${WKSAPCE_PATH}/cmake/zephyr.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# Declare project
project(app
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

# Add configure files
set(TX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)

# Add compile options
include_dirs(
    ${WKSAPCE_PATH}
    ${WKSAPCE_PATH}/basework/arch/arm
)
add_compile_options(
    -DSTM32H750xx=1
    -DUSE_HAL_DRIVER=1
    -DUSE_FULL_LL_DRIVER=1

    -Wno-unused-parameter
)

add_executable(${PROJECT_NAME}
    main.c
    startup.c
)

# Add subdirectory
add_subdirectory(${WKSAPCE_PATH}/rtos rtos)
add_subdirectory(${WKSAPCE_PATH}/stm32/Drivers drivers)
add_subdirectory(${WKSAPCE_PATH}/basework basework)

# Link target
collect_link_libraries(libs ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} 
    -Wl,--start-group
    ${libs}
    -Wl,--end-group
)
