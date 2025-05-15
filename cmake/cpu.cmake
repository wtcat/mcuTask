# Name of the target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ${THREADX_ARCH})

if (${THREADX_ARCH} STREQUAL "linux")
    message(STATUS "Linux simualtor")
    include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
else()

if (${THREADX_ARCH} STREQUAL "cortex_m7")
    set(MCPU_FLAGS "-mthumb -mcpu=cortex-m7")
    set(VFP_FLAGS  "-mfloat-abi=hard -mfpu=fpv5-d16")
elseif (${THREADX_ARCH} STREQUAL "cortex_m4")
    set(MCPU_FLAGS "-mthumb -mcpu=cortex-m4")
    set(VFP_FLAGS  "-mfloat-abi=hard -mfpu=fpv4-sp-d16")
else()
    message(FATAL_ERROR "Invalid cpu architecture: ${THREADX_ARCH}")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)
endif()

# Threadx
set(THREADX_TOOLCHAIN "gnu")
