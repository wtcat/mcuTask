# Drivers 

if (CONFIG_UART_ASYNC_API)
target_sources(board_shared
    PRIVATE
    uart_async_rx.c
)
endif()
