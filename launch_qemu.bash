#!/bin/bash

# Launch qemu 
qemu-system-arm -kernel pivt.elf -cpu arm1176 -m 4096 -M raspi -no-reboot -serial stdio -append ""


