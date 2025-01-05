# Boards shared 

include_dirs(.)

target_sources(${PROJECT_NAME}
    PRIVATE
    platform.c
    taskrunner.c
    device.c
    sysinit.c
    object_pool.c
)

if (NOT CONFIG_SIMULATOR)
target_sources(${PROJECT_NAME}
    PRIVATE
    init_array.c
    cstub.c
    irq.c
    nanosleep.c
)
endif()

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

if (CONFIG_CPLUSPLUS)
    target_sources(${PROJECT_NAME}
        PRIVATE
        cppnew.cc
    )
endif()
