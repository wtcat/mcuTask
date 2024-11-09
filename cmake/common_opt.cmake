#############################
# Set common compile options
#############################
# set(CC_FLAGS   "-O2 -Wall -Werror -Wextra -Werror -fpie -fno-plt ")
set(CC_FLAGS   "-std=gnu11 -O2 -Wall -Werror -Wextra -ffunction-sections -fdata-sections")
set(SPEC_FLAGS "--specs=nosys.specs")
# set(LD_FLAGS   "-Wl,--entry=_this_module -Wl,-Map=${CMAKE_BINARY_DIR}/app.map -Wl,-T ${CMAKE_CURRENT_LIST_DIR}/linker.ld -Wl,-nostdlib -nostartfiles")
set(LD_FLAGS   "-Wl,-Map=${CMAKE_BINARY_DIR}/app.map -Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

#######################
# Post command
#######################
macro(build_post name)
add_custom_command(
    TARGET ${name}
    POST_BUILD
    COMMAND ${OBJCOPY} -O binary ${name} ${name}.bin
    COMMAND ${OBJDUMP} -d ${name} > ${name}.lst
)
endmacro()