# Shell
target_sources(subsys
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cli.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_os.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_uart.c
)

