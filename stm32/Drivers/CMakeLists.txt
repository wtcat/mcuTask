static_library(stm32)

add_library(stm32_drivers INTERFACE )
target_include_directories(stm32_drivers
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32H7xx/Include
)

file(GLOB HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32H7xx_HAL_Driver/Src/*.c)

target_sources(stm32
    PRIVATE
    ${HAL_SOURCES}
)

target_link_libraries(stm32 
    PUBLIC
    stm32_drivers
)
