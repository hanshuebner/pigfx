#
# Makefile
#

include ../circle-stdlib/Config.mk

CIRCLEHOME = ../circle-stdlib/libs/circle
NEWLIBDIR = ../circle-stdlib/install/$(NEWLIB_ARCH)

OBJS	= pivt.o Terminal.o Framebuffer.o Keyboard.o Logging.o

include $(CIRCLEHOME)/Rules.mk

CFLAGS += -I ../libvterm/include -I ../lru-cache/include
CPPFLAGS += -std=c++17
CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I ../circle-stdlib/include
LIBS := "$(NEWLIBDIR)/lib/libm.a" "$(NEWLIBDIR)/lib/libc.a" "$(NEWLIBDIR)/lib/libcirclenewlib.a" \
	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	$(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/net/libnet.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/libcircle.a \
	../build/libvterm.a

Keyboard.o: keymap.inc

keymap.inc: ../tools/keymap.txt
	@echo "Creating keymap"
	@perl ../tools/make-keymap.pl < $< > $@
