# Project name
TARGET			= feather2

# Set up device parameters
include stm32f030.mk
LDSCRIPT        = stm32f030x8.ld

# Define project files and settings
SRC             = $(wildcard *.c)
OBJS            = $(SRC:%.c=%.o)
INCLUDE         = 
DEFINES			= 

# Location of libopencm3
OPENCM3_DIR     = ../libopencm3

DEFINES			+= -D$(FAMILY)
INCLUDE         += -I$(OPENCM3_DIR)/include/
CFLAGS          += -Os -ggdb3 -ffunction-sections -fdata-sections
CXXFLAGS        += -Os -ggdb3 -fno-rtti -ffunction-sections -fdata-sections
CPPFLAGS        += -MD $(INCLUDE) $(DEFINES)
LDFLAGS         += -static -nostartfiles -fno-rtti
LDFLAGS			+= -Wl,--start-group -Wl,--end-group -Wl,--gc-sections
LDFLAGS			+= -lgcc -lc --specs=nosys.specs
LDLIBS          += -L$(OPENCM3_DIR)/lib
LDLIBS          += -l$(OPENCM3_LIB)


include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all

all: $(TARGET).elf $(TARGET).bin
	$(Q)arm-none-eabi-size $(TARGET).elf

clean:
	$(Q)$(RM) -rf $(TARGET).elf $(TARGET).bin $(OBJS)

flash:
	$(Q)st-flash --reset write $(TARGET).bin 0x8000000

include $(OPENCM3_DIR)/mk/gcc-rules.mk
