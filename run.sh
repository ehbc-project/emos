#!/bin/bash

qemu-system-i386 \
    -fda build/boot/arch/i686/pc/floppy.img \
    -usb \
    -device usb-kbd \
    -device usb-mouse \
    -device intel-hda \
    -device ide-cd \
    -boot a \
    -s
#   -device qemu-xhci \
#   -device usb-ehci \
#   -device ide-cf \
#   -device sd-card \
