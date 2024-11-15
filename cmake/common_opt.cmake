#############################
# Set common compile options
#############################
set(CC_FLAGS   "-std=gnu11 -O2 -Wall -Werror -Wextra")
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