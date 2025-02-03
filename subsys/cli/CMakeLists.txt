# Command line interface

static_library(cli)

target_sources(cli
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cli.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_os.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_help.c
)

# Memory command
target_sources(cli
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_thread.c
)

if (CONFIG_SUBSYS_FS)
target_sources(cli
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_mkfs.c
)
endif()