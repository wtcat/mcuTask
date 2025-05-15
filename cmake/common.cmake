#############################
# Set common compile options
#############################
set(C_FLAGS    "-std=gnu17")
set(CPP_FLAGS  "-std=c++20")
set(CC_FLAGS   "-Wall -Werror -Wextra")
set(SPEC_FLAGS "-nostartfiles -specs=nosys.specs")
set(LD_FLAGS   "-Wl,-Map=${CMAKE_BINARY_DIR}/mcutask.map ")

#######################
# Post command
#######################
macro(build_link name)
    collect_link_libraries(libs ${name})
    target_link_libraries(${name}
        -Wl,--whole-archive
        -Wl,--start-group
        ${libs}
        -Wl,--end-group
        -Wl,--no-whole-archive
    )
endmacro()

macro(build_post name)
    add_custom_command(
        TARGET ${name}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${name} ${name}.bin
        COMMAND ${CMAKE_OBJDUMP} -d ${name} > ${name}.lst
        COMMAND ${CMAKE_SIZE} ${name}
    )
endmacro()
