# Name of the target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m4)

set(MCPU_FLAGS "-mthumb -mcpu=cortex-m4")
set(VFP_FLAGS  "-mfloat-abi=hard -mfpu=fpv4-sp-d16")

include (${CMAKE_CURRENT_LIST_DIR}/common_opt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -std=gnu17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")

# Threadx
set(THREADX_ARCH "cortex_m4")
set(THREADX_TOOLCHAIN "gnu")