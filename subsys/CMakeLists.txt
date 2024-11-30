# Sub system

static_library(subsys)

target_sources(subsys
    PRIVATE
    empty.c
)

add_subdirectory_ifdef(CONFIG_SUBSYS_CLI cli)
