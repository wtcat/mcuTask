# CortexM shared

target_sources(${board_shared}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dwt.c
)

if (CONFIG_ICACHE OR CONFIG_DCACHE)
target_sources(${board_shared}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cache.c
)
endif()

if (CONFIG_CORTEXM_BACKTRACE)
target_sources(${board_shared}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/backtrace/cm_backtrace.c
)
endif()