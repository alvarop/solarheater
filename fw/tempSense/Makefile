OBJECTS=main.o fifo.o console.o

GCC_DIR =  ../../../ti/gcc/bin
SUPPORT_FILE_DIRECTORY = ../../../ti/gcc/include

DEVICE  = msp430g2533
CC      = $(GCC_DIR)/msp430-elf-gcc
GDB     = $(GCC_DIR)/msp430-elf-gdb

CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -O2 -g -std=c11
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY)

all: ${OBJECTS}
	$(CC) $(CFLAGS) $(LFLAGS) $? -o $(DEVICE).out

debug: all
	$(GDB) $(DEVICE).out
