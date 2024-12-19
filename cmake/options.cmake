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
option(CONFIG_CPLUSPLUS    "Enable c++ language support" OFF)
