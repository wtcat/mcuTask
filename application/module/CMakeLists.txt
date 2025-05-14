
add_llext_target(hello_world
    OUTPUT  ${PROJECT_BINARY_DIR}/hello_world.llext
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/hello_module.c
)
