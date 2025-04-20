# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_CRC
  ${CMAKE_CURRENT_SOURCE_DIR}/crc32c_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc32_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc24_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc16_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc8_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc7_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/crc4_sw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hex.c
)
