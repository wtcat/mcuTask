# Subsystem

add_subdirectory_ifdef(CONFIG_SUBSYS_CLI   cli)
add_subdirectory_ifdef(CONFIG_SUBSYS_SDIO  sdio)
add_subdirectory_ifdef(CONFIG_SUBSYS_FS    fs)
