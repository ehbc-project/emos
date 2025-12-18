#!/usr/bin/env bash

set -e

GDB_ARCH=
declare -a OBJECTS
declare -a GDB_OBJECT_FLAGS

print_usage() {
    echo "usage: $0 [-h] arch"
}

while getopts "hi" arg; do
    case $arg in
        h)
            print_usage
            exit 0
            ;;
        *)
            print_usage
            exit 1
            ;;
    esac
done

shift "$((OPTIND - 1))"
GDB_ARCH=$1

case $GDB_ARCH in
    i386)
        OBJECTS=(
            "build/boot/arch/i686/pc/bios/fdboot.elf"
            "build/boot/arch/i686/pc/bios/stage1.elf"
        )
        ;;
esac

for object in "${OBJECTS[@]}"; do
    GDB_OBJECT_FLAGS+=(--eval-command="add-symbol-file $object")
done

shift
"$GDB_ARCH-elf-gdb" \
    --eval-command="set confirm off" \
    --eval-command="add-symbol-file build/boot/bootloader.elf" \
    --eval-command="add-symbol-file build/kernel/kernel.elf" \
    "${GDB_OBJECT_FLAGS[@]}" \
    --eval-command="set confirm on" \
    --command="./.gdbinit" \
    "$@"
