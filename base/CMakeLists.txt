static_library(base)

add_subdirectory(trace)
add_subdirectory(container)
add_subdirectory(lib)
add_subdirectory(sys)

target_sources(base
    PRIVATE
    bitops.c
    log.c
    fsm.c
)

if (CONFIG_EXPORT_SYMBOL)
    target_sources(base PRIVATE symbol.c)
endif()

add_kasan_trace(base)
