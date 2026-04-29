#!/bin/bash

# J-Link Flash Script for STM32H7
# Usage: ./flash.sh [build_type]
# Default build_type is Release

BUILD_TYPE=${1:-Release}
ELF_FILE="${PWD}/build/${BUILD_TYPE}/PiccoloMotorDriver.elf"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file not found: $ELF_FILE"
    echo "Please build the project first with: cmake --build build --config $BUILD_TYPE"
    exit 1
fi

echo "Flashing $ELF_FILE to STM32H7 using J-Link..."

JLinkExe -commanderscript /dev/stdin << EOF
r
h
loadfile "$ELF_FILE"
verifybin "$ELF_FILE", 0x08000000
r
g
q
EOF

echo "Flash complete!"