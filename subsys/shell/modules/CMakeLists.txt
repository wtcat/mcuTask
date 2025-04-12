# SPDX-License-Identifier: Apache-2.0


zephyr_library_sources_ifdef(CONFIG_MEMORY_SHELL
  ${CMAKE_CURRENT_SOURCE_DIR}/shell_mem.c
)
