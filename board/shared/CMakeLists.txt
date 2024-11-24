# Boards shared 

include_dirs(.)

target_sources(${PROJECT_NAME}
    PRIVATE
    platform.c
    sysinit.c
    cstub.c
)
