# RTOS options
option(CONFIG_NETX      "Enable TCP/IP protocol stack" OFF)
option(CONFIG_FILEX     "Enable FAText filesystem" OFF)
option(CONFIG_USBX      "Enable USB protocol stack" OFF)
option(CONFIG_LEVELX    "Enable Levelx flash abstract layer" OFF)

# Subsystem
option(CONFIG_SUBSYS_CLI    "Enable command line" OFF)
option(CONFIG_SUBSYS_SD     "Enable SDIO/SD/MMC " OFF)

# Others
option(CONFIG_KMALLOC      "Enable kmalloc/kfree" ON)



# Add configure files
set(TX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tx_user.h)

if (CONFIG_FILEX)
    set(FILEX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/port)
    set(FX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/fx_user.h)
endif()
if (CONFIG_NETX)
    set(NETXDUO_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/)
    set(NX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/nx_user.h)
endif()
if (CONFIG_USBX)
    set(USBX_CUSTOM_PORT ${CMAKE_CURRENT_SOURCE_DIR}/)
    set(UX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/port/ux_user.h)
endif()

