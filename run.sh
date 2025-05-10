#!/bin/bash

qemu-system-x86_64 -fda build/boot/bootloader.img \
    -device qemu-xhci \
    -device usb-ehci \
    -usb \
    -device intel-hda \
    -device ide-cd \
    -s
#   -device ide-cf \
#   -device ide-hd \
#   -device sd-card \
