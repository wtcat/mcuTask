# Virtual filesystem

static_library(fs)

target_sources(fs
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_null.c
)
