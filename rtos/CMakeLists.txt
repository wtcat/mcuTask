# RTOS compnent cmake
set(RTOS_CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include_dirs(${PROJECT_BINARY_DIR})

include(${RTOS_CMAKE_DIR}/ThreadX.cmake)
target_sources(threadx
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/threadx_symbol.c
)

if (CONFIG_FILEX)
    include(${RTOS_CMAKE_DIR}/FileX.cmake)
endif()

if (CONFIG_NETX)
    include(${RTOS_CMAKE_DIR}/Netxduo.cmake)
endif()

if (CONFIG_USBX)
    include(${RTOS_CMAKE_DIR}/UsbX.cmake)
endif()

if (CONFIG_LEVELX)
    include(${RTOS_CMAKE_DIR}/LevelX.cmake)
endif()
