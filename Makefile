
ARMGNU ?= arm-none-eabi
LIBVTERM_CFLAGS = -O0 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -mfloat-abi=soft -Iuspi/include -Ilibvterm/include
CFLAGS = -Wall -Wextra $(LIBVTERM_CFLAGS)
CXXFLAGS = -std=c++17

## Important!!! asm.o must be the first object to be linked!
OOB = 	asm.o pigfx.o uart.o irq.o hwutils.o timer.o framebuffer.o postman.o \
	console.o gfx.o dma.o uspios_wrapper.o \
	raspihwconfig.o stupid_timer.o binary_assets.o term.o \
	syscall_stubs.o

BUILD_DIR = build
SRC_DIR = src
BUILD_VERSION = $(shell git describe --all --long | cut -d "-" -f 3)

OBJS=$(patsubst %.o,$(BUILD_DIR)/%.o,$(OOB))

LIBUSPI=uspi/lib/libuspi.a
LIBVTERM=$(BUILD_DIR)/libvterm.a

all: pigfx.elf pigfx.hex kernel
	ctags src/

$(SRC_DIR)/pigfx_config.h: pigfx_config.h.in 
	@sed 's/\$$VERSION\$$/$(BUILD_VERSION)/g' pigfx_config.h.in > $(SRC_DIR)/pigfx_config.h
	@echo "Creating pigfx_config.h"

run: pigfx.elf
	./launch_qemu.bash

kernel: pigfx.img
	cp pigfx.img bin/kernel.img

debug: pigfx.elf
	cd JTAG && ./run_gdb.sh

dump: pigfx.elf
	@$(ARMGNU)-objdump --disassemble-zeroes -D pigfx.elf > pigfx.dump
	@echo "OBJDUMP $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	@$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.s 
	@$(ARMGNU)-as $< -o $@
	@echo "AS $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cc 
	@$(ARMGNU)-c++ $(CFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo "C++ $<"

$(LIBVTERM): $(patsubst libvterm/src/%.c,$(LIBVTERM)(%.o),$(wildcard libvterm/src/*.c))

libvterm/src/encoding/%.inc: libvterm/src/encoding/%.tbl
	perl -CSD libvterm/tbl2inc_c.pl $< > $@

$(LIBVTERM)(%.o): libvterm/src/%.c $(patsubst %.tbl,%.inc,$(wildcard libvterm/src/encoding/*.tbl))
	@echo "CC $*.c (libvterm)"
	@$(ARMGNU)-gcc -Ilibvterm/src $(LIBVTERM_CFLAGS) -Dstrncpy=uspi_strncpy -c $< -o $*.o
	@$(ARMGNU)-ar cq $(LIBVTERM) $*.o
	@$(RM) -f $*.o

%.hex : %.elf 
	@$(ARMGNU)-objcopy $< -O ihex $@
	@echo "OBJCOPY $< -> $@"

%.img : %.elf 
	@$(ARMGNU)-objcopy $< -O binary $@
	@echo "OBJCOPY $< -> $@"

pigfx.elf : $(SRC_DIR)/pigfx_config.h $(OBJS) $(LIBVTERM) $(LIBUSPI)
	@$(ARMGNU)-c++ -nostartfiles $(OBJS) $(LIBUSPI) $(LIBVTERM) -T memmap -o $@
	@echo "LD $@"

install: kernel
	cp bin/kernel.img /Volumes/PIGFX/
	diskutil umountdisk PIGFX

.PHONY clean :
	rm -f $(SRC_DIR)/pigfx_config.h
	rm -f $(BUILD_DIR)/*
	rm -f *.hex
	rm -f *.elf
	rm -f *.img
	rm -f *.dump
	rm -f tags
