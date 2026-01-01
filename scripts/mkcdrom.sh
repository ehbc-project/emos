#!/usr/bin/env bash

set -e

ARCH=
BOOTBIN_EXT=
OUTPUT=
PRESERVE_TEMP=false
CURRENT_IMAGE=
BOOT_IMAGE=

print_usage() {
    echo "usage: $0 [-a arch] [-hS] output"
}

while getopts "a:hSu" arg; do
    case $arg in
        a)
            ARCH=$OPTARG
            ;;
        h)
            print_usage
            exit 0
            ;;
        S)
            PRESERVE_TEMP=true
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
        ;;
    x86_64)
        BOOTBIN_EXT=X64
        ;;
    *)
        echo "$0: unknown architecture"
        exit 1
        ;;
esac

shift "$((OPTIND - 1))"
OUTPUT=$1

# El Torito floppy image
CURRENT_IMAGE=$(mktemp)
BOOT_IMAGE=$CURRENT_IMAGE
dd if=/dev/zero of="$CURRENT_IMAGE" bs=512 count=2880
mformat -i "$CURRENT_IMAGE" -B "build/boot/arch/$ARCH/pc/bios/fdboot.bin"
mcopy -i "$CURRENT_IMAGE" "build/boot/arch/$ARCH/pc/bios/stage1.bin" ::/STAGE1.$BOOTBIN_EXT
mcopy -i "$CURRENT_IMAGE" "build/boot/arch/$ARCH/pc/bios/bootloader.bin" ::/BOOTLDR.$BOOTBIN_EXT

if [ "${PRESERVE_TEMP}" = true ]; then
    cp "$CURRENT_IMAGE" "${OUTPUT%.*}.fd.img";
fi

if [ -d "./.mkcdrom.temp" ]; then
    rm -r ./.mkcdrom.temp
fi
mkdir ./.mkcdrom.temp
mkdir ./.mkcdrom.temp/config
mkdir ./.mkcdrom.temp/modules
cp "$BOOT_IMAGE" ./.mkcdrom.temp/boot.img
cp boot/config/boot.json ./.mkcdrom.temp/config/boot.json
cp build/boot/modules/bootemos/bootemos.mod ./.mkcdrom.temp/modules/bootemos.mod
cp build/boot/modules/helloworld/helloworld.mod ./.mkcdrom.temp/modules/helowrld.mod
cp build/boot/bootloader.map ./.mkcdrom.temp/bootldr.map
cp build/boot/unifont.bfn ./.mkcdrom.temp/unifont.bfn
cp disk/plchldr.bmp ./.mkcdrom.temp/plchldr.bmp
cp disk/unicode.txt ./.mkcdrom.temp/unicode.txt

mkdir ./.mkcdrom.temp/system
mkdir ./.mkcdrom.temp/system/kernel
mkdir ./.mkcdrom.temp/system/lib
mkdir ./.mkcdrom.temp/system/subsys
mkdir ./.mkcdrom.temp/system/services
mkdir ./.mkcdrom.temp/system/drivers
cp build/kernel/kernel.elf ./.mkcdrom.temp/system/kernel/kernel.elf

xorriso -as mkisofs -o "$OUTPUT" -b "/boot.img" ./.mkcdrom.temp

rm -r ./.mkcdrom.temp

rm "$BOOT_IMAGE"
