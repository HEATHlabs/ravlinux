RAV_CROSSTOOL = arm-rav-uclinux-uclibcgnueabi
AS	= $(RAV_CROSSTOOL)-as
CC    	= $(RAV_CROSSTOOL)-gcc
CXX    	= $(RAV_CROSSTOOL)-g++
AR    	= $(RAV_CROSSTOOL)-ar
LD    	= $(RAV_CROSSTOOL)-ld
DS    	= $(RAV_CROSSTOOL)-objdump
OC    	= $(RAV_CROSSTOOL)-objcopy

CFLAGS=-O2 -march=armv4t -std=c99 -Wall -I../include
ASFLAGS=-march=armv4t

all: $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC)

.PHONY : clean
clean:
		@rm -rfv *.o *.elf *.flt *.gdb *.dis *.map *.mem *.v


