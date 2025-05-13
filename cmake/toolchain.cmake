# Toolchain settings

set(CMAKE_C_COMPILER    ${CROSS_COMPILER}gcc)
set(CMAKE_CXX_COMPILER  ${CROSS_COMPILER}g++)
set(CMAKE_AS            ${CROSS_COMPILER}as)
set(CMAKE_AR            ${CROSS_COMPILER}ar)
set(CMAKE_LINKER        ${CROSS_COMPILER}ld)
set(CMAKE_OBJCOPY       ${CROSS_COMPILER}objcopy)
set(CMAKE_OBJDUMP       ${CROSS_COMPILER}objdump)
set(CMAKE_SIZE          ${CROSS_COMPILER}size)
set(CMAKE_NM            ${CROSS_COMPILER}nm)
set(CMAKE_RANLIB        ${CROSS_COMPILER}ranlib)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# this makes the test compiles use static library option so that we don't need to pre-set linker flags and scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS   "${MCPU_FLAGS} ${VFP_FLAGS} ${C_FLAGS} ${CC_FLAGS} ${SPEC_FLAGS} -fdata-sections -ffunction-sections" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} ${CPP_FLAGS} ${CC_FLAGS} -fdata-sections -ffunction-sections -fno-rtti -fno-exceptions" CACHE INTERNAL "cxx compiler flags")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "asm compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${MCPU_FLAGS} ${LD_FLAGS} -Wl,--no-warn-rwx-segments -Wl,--gc-sections" CACHE INTERNAL "exe link flags")

SET(CMAKE_C_FLAGS_DEBUG "-Og -g -ggdb3" CACHE INTERNAL "c debug compiler flags")
SET(CMAKE_CXX_FLAGS_DEBUG "-Og -g -ggdb3" CACHE INTERNAL "cxx debug compiler flags")
SET(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "asm debug compiler flags")

SET(CMAKE_C_FLAGS_RELEASE "-O3" CACHE INTERNAL "c release compiler flags")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE INTERNAL "cxx release compiler flags")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm release compiler flags")