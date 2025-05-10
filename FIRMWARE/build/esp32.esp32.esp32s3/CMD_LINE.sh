#!/usr/bin/env bash
set -euET -o pipefail

port_default=COM10
bootloader=FIRMWARE.ino.bootloader.bin
partitions=FIRMWARE.ino.partitions.bin
firmware=FIRMWARE.ino.bin

port=${1:-${port_default}}

cat<<EOF
================================================================================
port=[${port}]
bootloader=[${bootloader}]
partitions=[${partitions}]
firmware=[${firmware}]
================================================================================
EOF

python -m esptool --chip esp32s3 --port "${port}" --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode keep --flash_freq keep --flash_size keep 0x0 "${bootloader}" 0x8000 "${partitions}" 0x10000 "${firmware}" 
