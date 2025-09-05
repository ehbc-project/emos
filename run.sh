#!/usr/bin/env bash

QEMU=qemu-system-i386
CPU_TYPE=
MEM_SIZE=128M
BOOT_DRIVE=
OVMF_FLAGS=
declare -a DEVICES
declare -a DRIVES

while getopts "u" arg; do
    case $arg in
        u)
            OVMF_FLAGS="--bios /usr/local/share/edk2.git/ovmf-ia32/OVMF-pure-efi.fd"
            ;;
        *)  ;;
    esac
done

shift "$(($OPTIND - 1))"

case $1 in
    isapc)
        CPU_TYPE=486
        MACHINE_TYPE=isapc
        BOOT_DRIVE=d
        DEVICES=(
            "floppy,drive=rd0"
            "ide-hd,drive=fd0"
            "ide-cd,drive=rd1"
            isa-ide
            i8042
            ne2k_isa
            isa-vga
            sb16
            mc146818rtc
            pc-testdev
        )
        DRIVES=(
            "file=build/boot/floppy.img,id=rd0,if=none,format=raw"
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ;;
    q35)
        MACHINE_TYPE=pc
        BOOT_DRIVE=d
        DEVICES=(
            "nvme,drive=fd0,serial=1234"
            "ide-cd,drive=rd0"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            qemu-xhci
            sdhci-pci
            sd-card
            am53c974
            e1000
            pci-serial
            pci-testdev
            usb-kbd
            usb-mouse
        )
        DRIVES=(
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd0,if=none,format=raw"
        )
        ;;
    pc|"")
        MACHINE_TYPE=pc
        BOOT_DRIVE=a
        DEVICES=(
            "floppy,drive=rd0"
            "ide-hd,drive=fd0"
            "ide-cd,drive=rd1"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            qemu-xhci
            sdhci-pci
            sd-card
            am53c974
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            "file=build/boot/floppy.img,id=rd0,if=none,format=raw"
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ;;
    *)
        echo "Unknown machine type"
        exit 1
        ;;
esac

shift

QEMU_DEVICE_FLAGS=""
for device in "${DEVICES[@]}"; do
    QEMU_DEVICE_FLAGS+="-device $device "
done

QEMU_DRIVE_FLAGS=""
for drive in "${DRIVES[@]}"; do
    QEMU_DRIVE_FLAGS+="-drive $drive "
done

# shellcheck disable=2086
$QEMU \
    ${CPU_TYPE:+-cpu $CPU_TYPE} \
    $OVMF_FLAGS \
    -m $MEM_SIZE \
    -M $MACHINE_TYPE \
    -debugcon stdio -s \
    -boot $BOOT_DRIVE \
    $QEMU_DRIVE_FLAGS \
    $QEMU_DEVICE_FLAGS \
    "$@"
