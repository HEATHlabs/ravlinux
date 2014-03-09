TOOL=arm-none-eabi-
CC=$(TOOL)gcc
LD=$(TOOL)ld
AS=$(TOOL)as
OBJCOPY=$(TOOL)objcopy

CFLAGS=-march=armv4t -std=c99 -Wall 
LDFLAGS=
ASFLAGS=-march=armv4t


bootloader.bin : bootloader.elf
	${OBJCOPY} -O binary $< $@

bootloader.elf : shboot.o 
	${LD} -T shboot.ld $^ -o $@ ${LDFLAGS}

shboot.o : shboot.s
	${AS} ${ASFLAGS} $< -o $@

shboot.s : shboot.S
	$(CC) -E $< -o $@

.PHONY : clean
clean :
	rm -rf *.s *.o *.elf *.bin *.hex
