#!/usr/bin/env bash
#
# Restore a private factory 1 MB flash backup to the HSRCF405_TF via DFU.
#
# The factory backup is not included in this repository (it contains the
# unique MCU ID). Pass your own backup file:
#
#   ./tools/restore_betaflight.sh /path/to/original_firmware_backup.bin
#
set -euo pipefail

BACKUP="${1:-}"
if [ -z "$BACKUP" ] || [ ! -f "$BACKUP" ]; then
	echo "Usage: $0 /path/to/original_firmware_backup.bin" >&2
	exit 1
fi
if ! dfu-util -l 2>/dev/null | grep -q "Found DFU"; then
	echo "No DFU device found. Hold BOOT0 while plugging USB." >&2
	exit 1
fi

echo "Restoring factory backup: $BACKUP"
dfu-util -a 0 -s 0x08000000:leave -D "$BACKUP"
