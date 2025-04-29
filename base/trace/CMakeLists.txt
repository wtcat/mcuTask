if (CONFIG_KASAN)
static_library(trace)

add_library(kasan_if INTERFACE )
target_compile_options(kasan_if
    INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-fsanitize=kernel-address>
    $<$<CXX_COMPILER_ID:ARMClang>:-fsanitize=address>
)
target_sources(trace
    PRIVATE
    kasan.c
)
endif(CONFIG_KASAN)

if (CONFIG_FTRACE)
add_library(fntrace INTERFACE)
target_compile_options(fntrace
    INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-finstrument-functions>
)
endif(CONFIG_FTRACE)
