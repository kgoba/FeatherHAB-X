DEVICE		= stm32f030c8
#DEVICE		= stm32f103c8
#DEVICE     = stm32f446re

ARCH_FLAGS      += -mthumb
ARCH_FLAGS      += -mcpu=cortex-m0
ARCH_FLAGS      += -mfloat-abi=soft
FAMILY          = STM32F0
OPENCM3_LIB     = opencm3_stm32f0
