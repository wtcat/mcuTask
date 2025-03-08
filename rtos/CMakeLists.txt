include_dirs(${PROJECT_BINARY_DIR})

add_subdirectory(threadx)
add_subdirectory_ifdef(CONFIG_FILEX   filex)
add_subdirectory_ifdef(CONFIG_NETX    netxduo)
add_subdirectory_ifdef(CONFIG_USBX    usbx)
add_subdirectory_ifdef(CONFIG_LEVELX  levelx)
