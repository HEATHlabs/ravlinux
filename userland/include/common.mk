AMBER_CROSSTOOL = arm-shmac-linux-uclibcgnueabi


AS		= $(AMBER_CROSSTOOL)-as
CC    	= $(AMBER_CROSSTOOL)-gcc
CXX    	= $(AMBER_CROSSTOOL)-g++
AR    	= $(AMBER_CROSSTOOL)-ar
LD    	= $(AMBER_CROSSTOOL)-ld
DS    	= $(AMBER_CROSSTOOL)-objdump
OC    	= $(AMBER_CROSSTOOL)-objcopy

CFLAGS=-O3 -march=armv4t -std=c99 -Wall -I./
ASFLAGS=-march=armv4t

all: $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC)


#$(TGT) : $(SRC)
#	$(CC) $(CFLAGS) -o $(TGT) $(SRC)

.PHONY : clean
clean:
		@rm -rfv *.o *.elf *.flt *.gdb *.dis *.map *.mem *.v


