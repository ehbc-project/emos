#!/usr/bin/env bash

set -e

ARCH=
BOOTBIN_EXT=
UEFI_ARCH=
BOOT_TYPE=bios

print_usage() {
    echo "usage: $0 [-a arch] [-hu] image"
}

while getopts "a:hu" arg; do
    case $arg in
        a)
            ARCH=$OPTARG
            ;;
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

case $ARCH in
    i686)
        BOOTBIN_EXT=X86
        UEFI_ARCH=IA32
        ;;
    x86_64)
        BOOTBIN_EXT=X64
        UEFI_ARCH=X64
        ;;
    *)
        echo "$0: unknown architecture"
        exit 1
        ;;
esac

shift "$(($OPTIND - 1))"

IMAGE=$1
shift

dd if=/dev/zero of=part0.img bs=512 count=8190
case $BOOT_TYPE in
    uefi)
        mformat -i part0.img -H 63
        ;;
    bios)
        mformat -i part0.img -H 63 -R 63 -B build/boot/arch/$ARCH/pc/bios/fdboot.bin
        ;;
    *)
        echo "usage: $0 uefi|bios"
        exit 1
        ;;
esac
mcopy -s -i part0.img boot/config ::/CONFIG
mcopy -i part0.img build/boot/bootloader.map ::/BOOTLDR.MAP
mcopy -i part0.img build/boot/unifont.bfn ::/UNIFONT.BFN
mcopy -i part0.img disk/plchldr.bmp ::/PLCHLDR.BMP
mcopy -i part0.img disk/unicode.txt ::/UNICODE.TXT

dd if=/dev/zero of=part1.img bs=512 count=65520
build/tools/mkfs.afs/mkfs.afs -l "Label" -j 1M -N 2 -r 1 -s 2 -o 8253 -u $(uuidgen) -iv part1.img
# build/tools/afsimg/afsimg mkdir -i part1.img :/system
# build/tools/afsimg/afsimg mkdir -i part1.img :/system/kernel
# build/tools/afsimg/afsimg mkdir -i part1.img :/system/lib
# build/tools/afsimg/afsimg mkdir -i part1.img :/system/subsys
# build/tools/afsimg/afsimg mkdir -i part1.img :/system/services
# build/tools/afsimg/afsimg mkdir -i part1.img :/system/drivers
# build/tools/afsimg/afsimg mkdir -i part1.img :/users
# build/tools/afsimg/afsimg mkdir -i part1.img :/users/root
# build/tools/afsimg/afsimg mkdir -i part1.img :/packages
# build/tools/afsimg/afsimg mkdir -i part1.img :/temp
# build/tools/afsimg/afsimg copy -i part1.img build/kernel/kernel.elf :/system/kernel/kernel.elf

dd if=/dev/zero of=part2.img bs=512 count=131040
mformat -i part2.img -H 73773 -F
mmd -i part2.img ::/system
mmd -i part2.img ::/system/kernel
mmd -i part2.img ::/system/lib
mmd -i part2.img ::/system/subsys
mmd -i part2.img ::/system/services
mmd -i part2.img ::/system/drivers
mmd -i part2.img ::/users
mmd -i part2.img ::/users/root
mmd -i part2.img ::/packages
mmd -i part2.img ::/temp
mcopy -i part2.img build/kernel/kernel.elf ::/system/kernel/kernel.elf

case $BOOT_TYPE in
    uefi)
        mmd -i part0.img ::/EFI
        mmd -i part0.img ::/EFI/BOOT
        mcopy -i part0.img build/boot/arch/$ARCH/pc/uefi/bootloader.efi ::/EFI/BOOT/BOOT$UEFI_ARCH.EFI

        dd if=/dev/zero of=pt.img bs=512 count=63
        cat pt.img part0.img part1.img part2.img > "$IMAGE"
        cat <<'EOF' | gdisk "$IMAGE"
o
y
x
l
63
m
n
1
63
8252
ef00
n
2
8253
73772
B73C1AFA-E305-88F6-A9B4-FF47C0099DF7
n
3
73773
204779
F2B91CDC-E305-8B43-BDBE-5C2DD5526932
w
y
EOF
        ;;
    bios)
        mcopy -i part0.img build/boot/arch/$ARCH/pc/bios/bootloader.bin ::/BOOTLDR.$BOOTBIN_EXT
        python tools/injectbin/injectbin.py build/boot/arch/$ARCH/pc/bios/stage1.bin part0.img 512

        dd if=/dev/zero of=pt.img bs=512 count=62
        cat build/boot/arch/$ARCH/pc/bios/mbrboot.bin pt.img part0.img part1.img part2.img > "$IMAGE"

        cat <<'EOF' | fdisk -e "$IMAGE"
e 1
01
n
63
8190
f 1
e 2
78
n
8253
65520
edit 3
79
n
73773
131040
q
EOF
        ;;
    *)  ;;
esac

rm pt.img part0.img part1.img part2.img
