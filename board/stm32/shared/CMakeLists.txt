# STM32 shared 
static_library(stm32_shared)

target_sources(stm32_shared
    PRIVATE
    cortexm_irq.c
    cortexm_exception.c
)
