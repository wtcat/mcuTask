# cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

# Set up the project
# project(usbx
#     LANGUAGES C ASM
# )    
set(PROJECT_NAME "usbx")
set(SRCTREE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/usbx)

static_library(${PROJECT_NAME})
target_compile_options(${PROJECT_NAME}
    PRIVATE
    -Wno-array-bounds
)

add_dependencies(${PROJECT_NAME} threadx)
if (CONFIG_NETX)
    add_dependencies(${PROJECT_NAME} netxduo)
endif()
if (CONFIG_FILEX)
    add_dependencies(${PROJECT_NAME} filex)
endif()


if(NOT DEFINED THREADX_ARCH)
    message(FATAL_ERROR "Error: THREADX_ARCH not defined")
endif()
if(NOT DEFINED THREADX_TOOLCHAIN)
    message(FATAL_ERROR "Error: THREADX_TOOLCHAIN not defined")
endif()

# Define our target library and an alias for consumers
# add_library(${PROJECT_NAME})
# add_library("azrtos::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

# Define any required dependencies between this library and others
# target_link_libraries(${PROJECT_NAME} PUBLIC 
#     "azrtos::threadx"
#     "azrtos::filex"
#     "azrtos::netxduo"
# )

# A place for generated/copied include files
set(CUSTOM_INC_DIR ${PROJECT_BINARY_DIR})

# Pick up the port specific stuff first
if(DEFINED USBX_CUSTOM_PORT)
    add_subdirectory(${USBX_CUSTOM_PORT} usbx_port)
else()
    add_subdirectory(${SRCTREE_DIR}/ports/${THREADX_ARCH}/${THREADX_TOOLCHAIN})
endif()

# Then the common files
add_subdirectory(${SRCTREE_DIR}/common)


# If the user provided an override, copy it to the custom directory
if (NOT UX_USER_FILE)
    message(STATUS "Using default ux_user_config.h file")
    set(UX_USER_FILE ${SRCTREE_DIR}/../configs/ux_user_config.h)
else()
    message(STATUS "Using custom ux_user.h file from ${UX_USER_FILE}")
endif()
configure_file(${UX_USER_FILE} ${CUSTOM_INC_DIR}/ux_user.h COPYONLY)
# target_include_directories(${PROJECT_NAME} 
#     PUBLIC 
#     ${CUSTOM_INC_DIR}
# )

compile_definitions(UX_INCLUDE_USER_DEFINE_FILE)

# target_compile_definitions(${PROJECT_NAME} PUBLIC "UX_INCLUDE_USER_DEFINE_FILE" )
