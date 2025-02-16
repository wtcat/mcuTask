# Subsystem

add_subdirectory_ifdef(CONFIG_SUBSYS_CLI cli)
add_subdirectory_ifdef(CONFIG_SUBSYS_SD  sdio)
add_subdirectory_ifdef(CONFIG_SUBSYS_FS  fs)
