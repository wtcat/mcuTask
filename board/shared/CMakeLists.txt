# Boards shared 

set(board_shared ${PROJECT_NAME})

include_dirs(.)

set(TARGET_SRCS
    platform.c
    device.c
    sysinit.c
    object_pool.c
    init_array.c
    heap.c
    kmalloc.c
    malloc.c
    thread_foreach.c
)

if (CONFIG_HRTIMER)
    list(APPEND TARGET_SRCS nanosleep.c)
endif()

if (CONFIG_KMALLOC)
    list(APPEND TARGET_SRCS kmalloc.c)
endif()

if (CONFIG_CPLUSPLUS)
    list(APPEND TARGET_SRCS cppnew.cc)
endif()

if (CONFIG_MALLOC)
    list(APPEND TARGET_SRCS malloc.c)
endif()

if (CONFIG_TASK_RUNNER)
    list(APPEND TARGET_SRCS taskrunner.c)
endif()

if (NOT CONFIG_SIMULATOR)
    list(APPEND TARGET_SRCS cstub.c irq.c)
endif()

target_sources(${board_shared}
    PRIVATE
    ${TARGET_SRCS}
)

add_subdirectory(drivers)
add_subdirectory(cortexm)
