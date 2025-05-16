# Virtual filesystem

static_library(fs)

target_sources(fs
    PRIVATE
    fs.c
    fs_null.c
)

if (CONFIG_FILEX)
    target_sources(fs PRIVATE fs_filex.c)
endif()

if (CONFIG_FS_SHELL)
    target_sources(fs PRIVATE fs_shell.c)
endif()
