#!/bin/bash

qemu-system-i386 \
    -m 128M \
    -M q35 \
    -fda build/boot/floppy.img \
    -cdrom build/boot/cdrom.iso \
    -usb \
    -device usb-kbd \
    -device usb-mouse \
    -device intel-hda \
    -device qemu-xhci \
    -device usb-ehci \
    -device sdhci-pci \
    -device sd-card \
    -device am53c974 \
    -device e1000 \
    -device pci-serial \
    -device pci-testdev \
    -device nvme,drive=hd,serial=1234 \
    -drive file=disk.img,id=hd,if=none \
    -boot a \
    -debugcon stdio \
    -s

#    -hda build/boot/hd.img \
