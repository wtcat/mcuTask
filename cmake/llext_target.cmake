if (CONFIG_ARM)
    set(LLEXT_REMOVE_FLAGS
        -fno-pic
        -fno-pie
        -ffunction-sections
        -fdata-sections
        -flto=auto
        -fuse-linker-plugin
        -specs=nano.specs
        -Os
    )

    # Flags to be added to llext code compilation
    set(LLEXT_APPEND_FLAGS
        -mlong-calls
        -mthumb
    )
endif()