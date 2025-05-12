if(CONFIG_LLEXT)
  static_library(llext)

  target_sources(llext
    PRIVATE
    llext.c
    llext_mem.c
    llext_load.c
    llext_link.c
    llext_export.c
    llext_handlers.c
    buf_loader.c
    fs_loader.c
  )
  if (CONFIG_ARM)
  	target_sources(llext PRIVATE arm_elf.c)
  endif()
  if (CONFIG_LLEXT_SHELL)
    target_sources(llext PRIVATE shell.c)
  endif()
endif(CONFIG_LLEXT)
