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

add_kasan_trace(base)
