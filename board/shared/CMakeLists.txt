# Boards shared 

include_dirs(.)

target_sources(${PROJECT_NAME}
    PRIVATE
    platform.c
    sysinit.c
    cstub.c
    irq.c
)

if (CONFIG_KMALLOC)
target_sources(${PROJECT_NAME}
    PRIVATE
    kmalloc.c
)
endif()

if (CONFIG_MALLOC)
    target_sources(${PROJECT_NAME}
        PRIVATE
        malloc.c
    )
endif()
