#!/usr/bin/env bash
#
# Flash the HSRCF405_TF PX4 image through the STM32 system DFU bootloader.
#
# Enter DFU first: hold BOOT0, plug in USB, release BOOT0.
#
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMG="${1:-$DIR/firmware/hsrc_f405tf_px4_full_dfu.bin}"

if [ ! -f "$IMG" ]; then
	echo "Image not found: $IMG" >&2
	exit 1
fi
if ! command -v dfu-util >/dev/null 2>&1; then
	echo "dfu-util is not installed: sudo apt install dfu-util" >&2
	exit 1
fi
if ! dfu-util -l 2>/dev/null | grep -q "Found DFU"; then
	echo "No DFU device found. Hold BOOT0 while plugging USB." >&2
	exit 1
fi

echo "Flashing: $IMG"
dfu-util -a 0 -s 0x08000000:leave -D "$IMG"
echo "Done. The board reboots into PX4."
