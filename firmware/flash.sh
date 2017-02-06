#!/bin/bash
arm-none-eabi-size feather.elf
arm-none-eabi-objcopy -O binary feather.elf feather.bin
st-flash write feather.bin 0x8000000

