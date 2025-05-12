# Subsystem

add_subdirectory_ifdef(CONFIG_SUBSYS_SDIO  sdio)
add_subdirectory_ifdef(CONFIG_SUBSYS_FS    fs)
add_subdirectory_ifdef(CONFIG_SHELL        shell)
add_subdirectory_ifdef(CONFIG_MODBUS       modbus)
add_subdirectory_ifdef(CONFIG_LLEXT        llext)
