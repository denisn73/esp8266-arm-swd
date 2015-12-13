# Sample makefile for compiling small programs into URL fragments

NAME = patch

all: disassemble url

url: $(NAME).bin
	@echo
	@echo URL encoded program:
	@echo `hexdump -e '"0x%08x" "\n"' $<` | sed 's/ /\&.=/g'

disassemble: $(NAME).elf
	arm-none-eabi-objdump -d $<

$(NAME).bin: $(NAME).elf
	arm-none-eabi-objcopy $< -O binary $@

$(NAME).elf: $(NAME).cpp
	arm-none-eabi-gcc -nostdlib -mcpu=cortex-m0 -mthumb -Os -o $@ $<

clean:
	rm -f $(NAME).elf $(NAME).bin

.PHONY: clean all disassemble url
