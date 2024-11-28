# Boards shared 

include_dirs(.)

target_sources(${PROJECT_NAME}
    PRIVATE
    memory.c
    platform.c
    sysinit.c
    cstub.c
)
