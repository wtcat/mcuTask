zephyr_library_named(base)

zephyr_include_directories(
    cpu/cortexm
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

if (CONFIG_KASAN)
    add_library(kasan INTERFACE )
    target_compile_options(kasan
        INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:-fsanitize=kernel-address>
        $<$<CXX_COMPILER_ID:ARMClang>:-fsanitize=address>
    )
    target_link_libraries(base PRIVATE kasan)
endif(CONFIG_KASAN)

if (CONFIG_FTRACE)
    add_library(fntrace INTERFACE)
    target_compile_options(fntrace
        INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-finstrument-functions>
    )
    target_link_libraries(base PRIVATE fntrace)
endif()

zephyr_library_compile_options(
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-unused-parameter>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-implicit-fallthrough>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:ARMClang>>:-Wno-missing-field-initializers>
#    $<$<CXX_COMPILER_ID:GNU>:-Wno-cast-function-type>
)

zephyr_library_sources( 
    log.c
    fsm.c
)

add_subdirectory(container)
add_subdirectory(lib)
