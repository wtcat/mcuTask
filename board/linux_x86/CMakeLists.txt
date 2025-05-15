
# Add configure files
cmake_setvar(TX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)
if (CONFIG_FILEX)
    cmake_setvar(FILEX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/port)
    cmake_setvar(FX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/fx_user.h)
endif()
if (CONFIG_NETX)
    cmake_setvar(NETXDUO_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/port)
    cmake_setvar(NX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/nx_user.h)
endif()
if (CONFIG_USBX)
    cmake_setvar(USBX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/port)
    cmake_setvar(UX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/ux_user.h)
endif()

# Add compile options
include_dirs(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -fno-exceptions")

compile_options(
    -DCONFIG_HEADER_FILE="basework_config.h"
    -Wall
    -Wextra
    -fdata-sections 
    -ffunction-sections
    -Wno-unused-parameter
    -Wno-implicit-fallthrough
    -O0 
    -g
)

target_sources(${PROJECT_NAME}
    PRIVATE
    main.c
    ram_blkdev.c
)

# Linker options
target_link_options(${PROJECT_NAME}
    PRIVATE
    -Wl,-Map=${CMAKE_BINARY_DIR}/mcutask.map
    -Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)

# Link target
cmake_import_libarary(cpthread /usr/lib/x86_64-linux-gnu/libpthread.a)
cmake_import_libarary(crt /usr/lib/x86_64-linux-gnu/librt.a)
