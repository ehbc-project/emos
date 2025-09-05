#!/usr/bin/env bash

set -e

case $1 in
    uefi)
        dd if=/dev/zero of=part0.img bs=512 count=8190
        mformat -i part0.img -H 63
        mmd -i part0.img ::/EFI
        mmd -i part0.img ::/EFI/BOOT
        mcopy -i part0.img build/boot/arch/i686/pc/uefi/bootloader.efi ::/EFI/BOOT/BOOTIA32.EFI
        mcopy -s -i part0.img boot/config ::/CONFIG
        mcopy -i part0.img build/boot/bootloader.map ::/BOOTLDR.MAP
        mcopy -i part0.img build/boot/unifont.bfn ::/UNIFONT.BFN
        mcopy -i part0.img disk/plchldr.bmp ::/PLCHLDR.BMP
        mcopy -i part0.img disk/unicode.txt ::/UNICODE.TXT

        dd if=/dev/zero of=part1.img bs=512 count=65520
        build/tools/mkfs.afs/mkfs.afs -j 1M -N 2 -r 1 -s 2 -o 8253 -u $(uuidgen) -iv part1.img

        dd if=/dev/zero of=part2.img bs=512 count=131040
        mformat -i part2.img -H 73773 -F
        mcopy -i part2.img disk/unicode.txt ::/UNICODE.TXT

        dd if=/dev/zero of=mbr.img bs=512 count=63
        cat mbr.img part0.img part1.img part2.img > disk.img
        printf "o\ny\nx\nl\n63\nm\nn\n1\n63\n8252\nef00\nw\ny\n" | gdisk disk.img

        rm mbr.img part0.img part1.img part2.img
        ;;
    bios)
        dd if=/dev/zero of=part0.img bs=512 count=8190
        mformat -i part0.img -H 63 -R 63 -B build/boot/arch/i686/pc/bios/fdboot.bin
        mcopy -i part0.img build/boot/arch/i686/pc/bios/bootloader.bin ::/BOOTLDR.X86
        mcopy -s -i part0.img boot/config ::/CONFIG
        mcopy -i part0.img build/boot/bootloader.map ::/BOOTLDR.MAP
        mcopy -i part0.img build/boot/unifont.bfn ::/UNIFONT.BFN
        mcopy -i part0.img disk/plchldr.bmp ::/PLCHLDR.BMP
        mcopy -i part0.img disk/unicode.txt ::/UNICODE.TXT
        python tools/injectbin/injectbin.py build/boot/arch/i686/pc/bios/stage1.bin part0.img 512

        dd if=/dev/zero of=part1.img bs=512 count=65520
        build/tools/mkfs.afs/mkfs.afs -j 1M -N 2 -r 1 -s 2 -o 8253 -u $(uuidgen) -iv part1.img

        dd if=/dev/zero of=part2.img bs=512 count=131040
        mformat -i part2.img -H 73773 -F
        mcopy -i part2.img disk/unicode.txt ::/UNICODE.TXT

        dd if=/dev/zero of=mbr.img bs=512 count=62
        cat build/boot/arch/i686/pc/bios/mbrboot.bin mbr.img part0.img part1.img part2.img > disk.img
        printf "edit 1\n01\nn\n63\n8190\nflag 1\nedit 2\n72\nn\n8253\n65520\nedit 3\n0B\nn\n73773\n131040\nquit\n" | fdisk -e disk.img

        rm mbr.img part0.img part1.img part2.img
        ;;
    *)
        echo "usage: $0 uefi|bios"
        exit 1
        ;;
esac
