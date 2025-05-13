# cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# Set up the project
# project(threadx
#     LANGUAGES C ASM
# )
set(PROJECT_NAME "threadx")
set(SRCTREE_DIR ${CMAKE_CURRENT_LIST_DIR}/threadx)

static_library(${PROJECT_NAME})
lib_include_directories(
    ${SRCTREE_DIR}/common/inc
    ${SRCTREE_DIR}/ports/${THREADX_ARCH}/${THREADX_TOOLCHAIN}/inc
    ${CMAKE_CURRENT_BINARY_DIR}/custom_inc
)

if(NOT DEFINED THREADX_ARCH)
    message(FATAL_ERROR "Error: THREADX_ARCH not defined")
endif()
if(NOT DEFINED THREADX_TOOLCHAIN)
    message(FATAL_ERROR "Error: THREADX_TOOLCHAIN not defined")
endif()
message(STATUS "THREADX_ARCH: ${THREADX_ARCH}")
message(STATUS "THREADX_TOOLCHAIN: ${THREADX_TOOLCHAIN}")

# Define our target library and an alias for consumers
# add_library(${PROJECT_NAME})
# add_library("azrtos::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

# A place for generated/copied include files (no need to change)
set(CUSTOM_INC_DIR ${PROJECT_BINARY_DIR})

# Pick up the port specific variables and apply them
if(DEFINED THREADX_CUSTOM_PORT)
    add_subdirectory(${THREADX_CUSTOM_PORT} threadx_port)
else()
    add_subdirectory(${SRCTREE_DIR}/ports/${THREADX_ARCH}/${THREADX_TOOLCHAIN})
endif()

# Pick up the common stuff
add_subdirectory(${SRCTREE_DIR}/common)

# Define the FreeRTOS adaptation layer
# add_library(freertos-threadx EXCLUDE_FROM_ALL)
# target_include_directories(freertos-threadx
#     PUBLIC
#     ${SRCTREE_DIR}/utility/rtos_compatibility_layers/FreeRTOS
# )
# target_sources(freertos-threadx
#     PRIVATE
#     ${SRCTREE_DIR}/utility/rtos_compatibility_layers/FreeRTOS/tx_freertos.c
# )
# target_link_libraries(freertos-threadx PUBLIC threadx)

# If the user provided an override, copy it to the custom directory
if (NOT TX_USER_FILE)
    message(STATUS "Using default tx_user_config.h file")
    set(TX_USER_FILE ${SRCTREE_DIR}/../configs/tx_user_config.h)
else()
    message(STATUS "Using custom tx_user.h file from ${TX_USER_FILE}")
endif()    
configure_file(${TX_USER_FILE} ${CUSTOM_INC_DIR}/tx_user.h COPYONLY)
# target_include_directories(${PROJECT_NAME} 
#     PUBLIC 
#     ${CUSTOM_INC_DIR}
# )
# target_compile_definitions(${PROJECT_NAME} PUBLIC "TX_INCLUDE_USER_DEFINE_FILE" )
compile_definitions(TX_INCLUDE_USER_DEFINE_FILE)

# Utility
lib_include_directories(
    ${SRCTREE_DIR}/utility/execution_profile_kit
    ${SRCTREE_DIR}/utility/low_power
)
target_sources(${PROJECT_NAME}
    PRIVATE
    ${SRCTREE_DIR}/utility/execution_profile_kit/tx_execution_profile.c
    ${SRCTREE_DIR}/utility/low_power/tx_low_power.c
)

# Compatiblility
set(POSIX_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/utility/rtos_compatibility_layers/posix)
set(FREERTOS_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/utility/rtos_compatibility_layers/FreeRTOS)
set(OSEK_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/utility/rtos_compatibility_layers/OSEK)

if (CONFIG_TX_POSIX_API)
    include_dirs(${POSIX_SRC_PATH}/../)
    target_compile_options(${PROJECT_NAME}
        PRIVATE
        -Wno-maybe-uninitialized
        -D_SIGSET_T_DECLARED
        -D_CLOCKID_T_DECLARED
        -D_TIME_T_DECLARED
        -D_SYS__PTHREADTYPES_H_
        -D_SYS__TIMEVAL_H_
        -D_SYS_TIMESPEC_H_
        -D_SYS_SELECT_H
    )
    target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${POSIX_SRC_PATH}
    )

    target_sources(${PROJECT_NAME}
        PRIVATE
        ${POSIX_SRC_PATH}/px_pth_attr_setschedpolicyl.c
        ${POSIX_SRC_PATH}/px_mx_unlock.c
        ${POSIX_SRC_PATH}/px_pth_setcancelstate.c
        ${POSIX_SRC_PATH}/px_sem_init.c
        ${POSIX_SRC_PATH}/px_mx_init.c
        ${POSIX_SRC_PATH}/px_sig_fillset.c
        ${POSIX_SRC_PATH}/px_pth_attr_getschedpolicy.c
        ${POSIX_SRC_PATH}/px_mx_lock.c
        # ${POSIX_SRC_PATH}/posix_signal_sigmask_test.c
        # ${POSIX_SRC_PATH}/posix_signal_self_send_test.c
        ${POSIX_SRC_PATH}/px_pth_set_default_pthread_attr.c
        ${POSIX_SRC_PATH}/px_pth_create.c
        ${POSIX_SRC_PATH}/px_pth_cancel.c
        ${POSIX_SRC_PATH}/px_mq_arrange_msg.c
        ${POSIX_SRC_PATH}/px_error.c
        ${POSIX_SRC_PATH}/px_memory_allocate.c
        ${POSIX_SRC_PATH}/px_sem_open.c
        ${POSIX_SRC_PATH}/px_mx_timedlock.c
        ${POSIX_SRC_PATH}/px_pth_attr_getstackaddr.c
        ${POSIX_SRC_PATH}/px_pth_attr_getinheritsched.c
        ${POSIX_SRC_PATH}/px_pth_once.c
        ${POSIX_SRC_PATH}/px_mx_attr_getprotocol.c
        ${POSIX_SRC_PATH}/px_sig_delset.c
        ${POSIX_SRC_PATH}/px_mx_attr_setpshared.c
        ${POSIX_SRC_PATH}/px_mq_attr_init.c
        ${POSIX_SRC_PATH}/px_sem_find_sem.c
        ${POSIX_SRC_PATH}/px_pth_attr_init.c
        ${POSIX_SRC_PATH}/px_pth_attr_setstackaddr.c
        ${POSIX_SRC_PATH}/px_pth_attr_setstack.c
        ${POSIX_SRC_PATH}/px_mx_attr_destroy.c
        ${POSIX_SRC_PATH}/px_sched_get_prio.c
        ${POSIX_SRC_PATH}/px_mq_unlink.c
        ${POSIX_SRC_PATH}/px_mx_trylock.c
        ${POSIX_SRC_PATH}/px_cond_wait.c
        ${POSIX_SRC_PATH}/px_sleep.c
        ${POSIX_SRC_PATH}/px_cond_destroy.c
        ${POSIX_SRC_PATH}/px_pth_getcanceltype.c
        ${POSIX_SRC_PATH}/px_cond_signal.c
        ${POSIX_SRC_PATH}/px_mq_get_new_queue.c
        ${POSIX_SRC_PATH}/px_sem_post.c
        ${POSIX_SRC_PATH}/px_sem_set_sem_name.c
        ${POSIX_SRC_PATH}/px_mq_send.c
        # ${POSIX_SRC_PATH}/posix_signal_resume_thread_test.c
        ${POSIX_SRC_PATH}/px_pth_self.c
        ${POSIX_SRC_PATH}/px_mq_get_queue_desc.c
        # ${POSIX_SRC_PATH}/posix_demo.c
        ${POSIX_SRC_PATH}/px_sig_addset.c
        ${POSIX_SRC_PATH}/px_mq_putback_queue.c
        ${POSIX_SRC_PATH}/px_sem_wait.c
        ${POSIX_SRC_PATH}/px_pth_attr_setschedparam.c
        ${POSIX_SRC_PATH}/px_mx_set_default_mutexattr.c
        ${POSIX_SRC_PATH}/px_pth_attr_setinheritsched.c
        ${POSIX_SRC_PATH}/px_sem_getvalue.c
        ${POSIX_SRC_PATH}/px_pth_attr_setdetachstate.c
        ${POSIX_SRC_PATH}/px_pth_setschedparam.c
        ${POSIX_SRC_PATH}/px_cond_init.c
        ${POSIX_SRC_PATH}/px_mx_attr_setprotocol.c
        ${POSIX_SRC_PATH}/px_mq_priority_search.c
        ${POSIX_SRC_PATH}/px_in_thread_context.c
        ${POSIX_SRC_PATH}/px_pth_detach.c
        ${POSIX_SRC_PATH}/px_sem_unlink.c
        ${POSIX_SRC_PATH}/px_mq_create.c
        # ${POSIX_SRC_PATH}/posix_signal_suspended_thread_test.c
        ${POSIX_SRC_PATH}/px_mx_attr_getpshared.c
        ${POSIX_SRC_PATH}/px_mq_receive.c
        ${POSIX_SRC_PATH}/px_mx_attr_gettype.c
        ${POSIX_SRC_PATH}/px_pth_attr_destroy.c
        ${POSIX_SRC_PATH}/px_nanosleep.c
        ${POSIX_SRC_PATH}/px_px_initialize.c
        ${POSIX_SRC_PATH}/px_pth_attr_getstack.c
        ${POSIX_SRC_PATH}/px_pth_attr_getdetachstate.c
        ${POSIX_SRC_PATH}/px_pth_getschedparam.c
        ${POSIX_SRC_PATH}/px_pth_join.c
        ${POSIX_SRC_PATH}/px_pth_equal.c
        ${POSIX_SRC_PATH}/px_sem_reset.c
        ${POSIX_SRC_PATH}/px_sem_destroy.c
        ${POSIX_SRC_PATH}/px_clock_settime.c
        # ${POSIX_SRC_PATH}/px_pth_testcancel.c
        ${POSIX_SRC_PATH}/px_pth_setcanceltype.c
        ${POSIX_SRC_PATH}/px_clock_getres.c
        ${POSIX_SRC_PATH}/px_sched_yield.c
        ${POSIX_SRC_PATH}/px_system_manager.c
        ${POSIX_SRC_PATH}/px_sem_get_new_sem.c
        ${POSIX_SRC_PATH}/px_pth_sigmask.c
        ${POSIX_SRC_PATH}/px_abs_time_to_rel_ticks.c
        ${POSIX_SRC_PATH}/px_mx_destroy.c
        ${POSIX_SRC_PATH}/px_pth_exit.c
        ${POSIX_SRC_PATH}/px_mq_reset_queue.c
        ${POSIX_SRC_PATH}/px_mq_open.c
        ${POSIX_SRC_PATH}/px_cond_timedwait.c
        ${POSIX_SRC_PATH}/px_clock_gettime.c
        ${POSIX_SRC_PATH}/px_pth_attr_setstacksize.c
        ${POSIX_SRC_PATH}/px_pth_kill.c
        ${POSIX_SRC_PATH}/px_pth_attr_getschedparam.c
        ${POSIX_SRC_PATH}/px_mx_attr_initi.c
        # ${POSIX_SRC_PATH}/posix_signal_sigwait_test.c
        ${POSIX_SRC_PATH}/px_pth_init.c
        # ${POSIX_SRC_PATH}/posix_signal_nested_test.c
        ${POSIX_SRC_PATH}/px_sig_signal.c
        ${POSIX_SRC_PATH}/px_sig_wait.c
        ${POSIX_SRC_PATH}/px_mq_find_queue.c
        ${POSIX_SRC_PATH}/px_memory_release.c
        ${POSIX_SRC_PATH}/px_sem_trywait.c
        ${POSIX_SRC_PATH}/px_mq_close.c
        ${POSIX_SRC_PATH}/px_pth_attr_getstacksize.c
        ${POSIX_SRC_PATH}/px_cond_broadcast.c
        ${POSIX_SRC_PATH}/px_sem_close.c
        ${POSIX_SRC_PATH}/px_mx_attr_settype.c
        ${POSIX_SRC_PATH}/px_pth_yield.c
        ${POSIX_SRC_PATH}/px_mq_queue_init.c
        ${POSIX_SRC_PATH}/px_sig_emptyset.c
        ${POSIX_SRC_PATH}/px_internal_signal_dispatch.c
        ${POSIX_SRC_PATH}/px_mq_queue_delete.c
    )
endif(CONFIG_TX_POSIX_API)

if (CONFIG_TX_FREERTOS_API)
    include_dirs(${FREERTOS_SRC_PATH})
    target_sources(${PROJECT_NAME}
        PRIVATE
        ${FREERTOS_SRC_PATH}/tx_freertos.c
    )
endif(CONFIG_TX_FREERTOS_API)

if (CONFIG_TX_OSEK_API)
    include_dirs(${OSEK_SRC_PATH})
    target_sources(${PROJECT_NAME}
        PRIVATE
        ${OSEK_SRC_PATH}/tx_osek.c
    )
endif(CONFIG_TX_OSEK_API)

# Enable a build target that produces a ZIP file of all sources
# set(CPACK_SOURCE_GENERATOR "ZIP")
# set(CPACK_SOURCE_IGNORE_FILES
#   \\.git/
#   \\.github/
#   _build/
#   \\.git
#   \\.gitattributes
#   \\.gitignore
#   ".*~$"
# )
# set(CPACK_VERBATIM_VARIABLES YES)
# include(CPack)