#############################
# Set common compile options
#############################
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(C_FLAGS    "-std=gnu17")
set(CPP_FLAGS  "-std=c++20")
set(CC_FLAGS   "-Wall -Werror -Wextra -Os")
set(SPEC_FLAGS "--specs=nosys.specs")
set(LD_FLAGS   "-Wl,-Map=${CMAKE_BINARY_DIR}/mcutask.map -Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

#######################
# Post command
#######################
macro(build_post name)
    add_custom_command(
        TARGET ${name}
        POST_BUILD
        COMMAND ${OBJCOPY} -O binary ${name} ${name}.bin
        COMMAND ${OBJDUMP} -d ${name} > ${name}.lst
        COMMAND ${SIZE} ${name}
    )
endmacro()
