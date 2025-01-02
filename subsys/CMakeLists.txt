# Subsystem

add_subdirectory_ifdef(CONFIG_SUBSYS_CLI cli)
add_subdirectory_ifdef(CONFIG_SUBSYS_SD  sd)
add_subdirectory_ifdef(CONFIG_SUBSYS_FS  fs)
