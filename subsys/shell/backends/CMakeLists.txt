# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(
  CONFIG_SHELL_BACKEND_SERIAL
  ${CMAKE_CURRENT_SOURCE_DIR}/shell_uart.c
)

zephyr_library_sources_ifdef(
  CONFIG_SHELL_BACKEND_DUMMY
  ${CMAKE_CURRENT_SOURCE_DIR}/shell_dummy.c
)

zephyr_library_sources_ifdef(
  CONFIG_SHELL_BACKEND_RTT
  ${CMAKE_CURRENT_SOURCE_DIR}/shell_rtt.c
)
