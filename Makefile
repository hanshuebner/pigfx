
ARMGNU ?= arm-none-eabi
ARCH = -march=armv6j -mtune=arm1176jzf-s -mfloat-abi=soft
LIBVTERM_CFLAGS = $(ARCH) -O0 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -Iuspi/include -Ilibvterm/include
CFLAGS = -Wall -Wextra $(LIBVTERM_CFLAGS)
CXXFLAGS = -std=c++17 -Ilru-cache/include
DEPFLAGS = -MT $@ -MMD -MP -MF $@.d

## asm must be the first module
MODULES = asm uart irq hwutils timer fb postman console dma	\
	uspios_wrapper raspihwconfig stupid_timer binary_assets	\
	syscall_stubs Framebuffer Terminal Keyboard pivt

BUILD_DIR = build
SRC_DIR = src
BUILD_VERSION = $(shell git describe --all --long | cut -d "-" -f 3)

OBJS=$(patsubst %,$(BUILD_DIR)/%.o,$(MODULES))

LIBUSPI=uspi/lib/libuspi.a
LIBVTERM=$(BUILD_DIR)/libvterm.a

all: pivt.elf pivt.hex kernel
	ctags src/

$(SRC_DIR)/keymap.inc: keymap.txt
	@echo "Creating keymap"
	@perl make-keymap.pl < $< > $@

run: pivt.elf
	./launch_qemu.bash

kernel: pivt.img
	cp pivt.img bin/kernel.img

debug: pivt.elf
	cd JTAG && ./run_gdb.sh

dump: pivt.elf
	@echo "OBJDUMP $<"
	@$(ARMGNU)-objdump --disassemble-zeroes -D pivt.elf > pivt.dump

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	@echo "CC $<"
	@$(ARMGNU)-gcc $(DEPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.s 
	@echo "AS $<"
	@$(ARMGNU)-as $< -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cc 
	@echo "C++ $<"
	@$(ARMGNU)-c++ $(DEPFLAGS) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(LIBVTERM): $(patsubst libvterm/src/%.c,$(LIBVTERM)(%.o),$(wildcard libvterm/src/*.c))

libvterm/src/encoding/%.inc: libvterm/src/encoding/%.tbl
	perl -CSD libvterm/tbl2inc_c.pl $< > $@

$(LIBVTERM)(%.o): libvterm/src/%.c $(patsubst %.tbl,%.inc,$(wildcard libvterm/src/encoding/*.tbl))
	@echo "CC $*.c (libvterm)"
	@$(ARMGNU)-gcc -Ilibvterm/src $(LIBVTERM_CFLAGS) -Dstrncpy=uspi_strncpy -c $< -o $*.o
	@$(ARMGNU)-ar cq $(LIBVTERM) $*.o
	@$(RM) -f $*.o

$(LIBUSPI):
	@echo "COMPILE libuspi"
	cd uspi && ln -sf ../uspi-config.mk Config.mk && cd lib && make

%.hex : %.elf 
	@echo "OBJCOPY $< -> $@"
	@$(ARMGNU)-objcopy $< -O ihex $@

%.img : %.elf 
	@echo "OBJCOPY $< -> $@"
	@$(ARMGNU)-objcopy $< -O binary $@

pivt.elf : $(OBJS) $(LIBVTERM) $(LIBUSPI)
	@echo "LD $@"
	@$(ARMGNU)-c++ -nostartfiles $(OBJS) $(LIBUSPI) $(LIBVTERM) -T memmap -o $@

install: kernel
	cp bin/kernel.img /Volumes/PIVT/
	diskutil umountdisk PIVT

clean:
	cd uspi/lib && make clean
	rm -f $(BUILD_DIR)/*
	rm -f *.hex
	rm -f *.elf
	rm -f *.img
	rm -f *.dump
	rm -f tags

.PHONY: $(LIBUSPI)

src/keymap.inc: keymap.txt make-keymap.pl

include $(wildcard build/*.d)
