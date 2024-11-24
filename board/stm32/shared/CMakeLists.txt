# STM32 shared 

target_sources(${PROJECT_NAME}
    PRIVATE
    irq.c
    armv7m_exception.c
)
