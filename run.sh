#!/usr/bin/env bash

set -e

QEMU_ARCH=
QEMU_MACHINE=
OVMF_PATH=
CPU_TYPE=
BOOT_TYPE=bios
MEM_SIZE=128M
ADDITIONAL_FLAGS=
declare -a DEVICES
declare -a DRIVES

print_usage() {
    echo "usage: $0 [-hu] machine"
}

while getopts "hu" arg; do
    case $arg in
        h)
            print_usage
            exit 0
            ;;
        u)
            BOOT_TYPE=uefi
            ;;
        *)
            print_usage
            exit 1
            ;;
    esac
done

shift "$(($OPTIND - 1))"

QEMU_MACHINE=$1
shift

case $QEMU_MACHINE in
    isapc-i386)
        QEMU_ARCH=i386
        CPU_TYPE=486
        MACHINE_TYPE=isapc
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
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    q35-i386)
        QEMU_ARCH=i386
        MACHINE_TYPE=q35
        DEVICES=(
            "nvme,drive=fd0,serial=1234"
            "ide-cd,drive=rd0"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
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
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    pc-i386)
        QEMU_ARCH=i386
        MACHINE_TYPE=pc
        DEVICES=(
            # "floppy,drive=rd0"
            "ide-hd,drive=fd0"
            "ide-hd,drive=fd1"
            # "ide-cd,drive=rd1"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            sdhci-pci
            sd-card
            am53c974
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            # "file=build/boot/floppy.img,id=rd0,if=none,format=raw"
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=disk2.img,id=fd1,if=none,format=raw"
            # "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    isapc-x86_64)
        QEMU_ARCH=x86_64
        MACHINE_TYPE=isapc
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
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    q35-x86_64)
        QEMU_ARCH=x86_64
        MACHINE_TYPE=q35
        DEVICES=(
            "nvme,drive=fd0,serial=1234"
            "ide-cd,drive=rd0"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
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
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    pc-x86_64)
        QEMU_ARCH=x86_64
        MACHINE_TYPE=pc
        DEVICES=(
            # "floppy,drive=rd0"
            "ide-hd,drive=fd0"
            "ide-cd,drive=rd1"
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            sdhci-pci
            sd-card
            am53c974
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            # "file=build/boot/floppy.img,id=rd0,if=none,format=raw"
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ADDITIONAL_FLAGS="-debugcon stdio"
        ;;
    virt-arm)
        QEMU_ARCH=arm
        CPU_TYPE=max
        MACHINE_TYPE=virt
        DEVICES=(
            virtio-scsi-pci
            "scsi-hd,drive=fd0"
            "scsi-cd,drive=rd1"
            virtio-gpu-pci
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            usb-kbd
            usb-mouse
            sdhci-pci
            sd-card
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ;;
    virt-aarch64)
        QEMU_ARCH=aarch64
        CPU_TYPE=max
        MACHINE_TYPE=virt
        DEVICES=(
            virtio-scsi-pci
            "scsi-hd,drive=fd0"
            "scsi-cd,drive=rd1"
            virtio-gpu-pci
            intel-hda
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            usb-kbd
            usb-mouse
            sdhci-pci
            sd-card
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ;;
    mac99-ppc64)
        QEMU_ARCH=ppc64
        MACHINE_TYPE=mac99
        DEVICES=(
            # "floppy,drive=rd0"
            am53c974
            "scsi-hd,drive=fd0"
            "scsi-cd,drive=rd1"
            ES1370
            qemu-xhci
            ich9-usb-uhci6
            usb-ehci
            sdhci-pci
            sd-card
            e1000
            pci-serial
            pci-testdev
        )
        DRIVES=(
            # "file=build/boot/floppy.img,id=rd0,if=none,format=raw"
            "file=disk.img,id=fd0,if=none,format=raw"
            "file=build/boot/cdrom.iso,id=rd1,if=none,format=raw"
        )
        ;;
    *)
        echo "Unknown machine type"
        exit 1
        ;;
esac

if [ $BOOT_TYPE = "uefi" ]; then
    case $QEMU_ARCH in
        i386)
            OVMF_PATH="/usr/local/share/edk2.git/ovmf-ia32/OVMF-pure-efi.fd"
            ;;
        x86_64)
            OVMF_PATH="/usr/local/share/edk2.git/ovmf-x64/OVMF-pure-efi.fd"
            ;;
        arm)
            OVMF_PATH="/usr/local/share/edk2.git/arm/QEMU_EFI.fd"
            ;;
        aarch64)
            OVMF_PATH="/usr/local/share/edk2.git/aarch64/QEMU_EFI.fd"
            ;;
        *)
            echo "$0: unknown architecture"
            exit 1
            ;;
    esac
fi

QEMU_DEVICE_FLAGS=""
for device in "${DEVICES[@]}"; do
    QEMU_DEVICE_FLAGS+="-device $device "
done

QEMU_DRIVE_FLAGS=""
for drive in "${DRIVES[@]}"; do
    QEMU_DRIVE_FLAGS+="-drive $drive "
done

# shellcheck disable=2086
qemu-system-$QEMU_ARCH \
    ${CPU_TYPE:+-cpu $CPU_TYPE} \
    ${OVMF_PATH:+-bios $OVMF_PATH} \
    -m $MEM_SIZE \
    -M $MACHINE_TYPE \
    -s \
    $QEMU_DRIVE_FLAGS \
    $QEMU_DEVICE_FLAGS \
    $ADDITIONAL_FLAGS \
    "$@"
