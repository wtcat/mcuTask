# SD card

static_library(sd)

target_sources(sd
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sd_ops.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdmmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mmc.c
)
