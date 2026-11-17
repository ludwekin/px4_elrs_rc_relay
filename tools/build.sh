#!/usr/bin/env bash
#
# Build the HSRCF405_TF PX4 firmware.
#
#   ./tools/build.sh
#
# Environment:
#   PX4_DIR   existing PX4-Autopilot tree (default: ~/PX4-Autopilot)
#   PX4_REF   PX4 git ref to clone (default: v1.15.3)
#
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PX4_DIR="${PX4_DIR:-$HOME/PX4-Autopilot}"
PX4_REF="${PX4_REF:-v1.15.3}"
BOARD_SRC="$REPO_DIR/boards/hsrc/f405tf"
BOARD_DST="$PX4_DIR/boards/hsrc/f405tf"

if [ ! -d "$PX4_DIR/.git" ]; then
	echo "Cloning PX4 $PX4_REF into $PX4_DIR ..."
	git clone --branch "$PX4_REF" --recursive https://github.com/PX4/PX4-Autopilot.git "$PX4_DIR"
fi

echo "Updating submodules ..."
git -C "$PX4_DIR" submodule update --init --recursive

# PX4's version header looks for a NuttX tag. Shallow/partial clones do not
# fetch tags, so create the expected local tag if needed.
if ! git -C "$PX4_DIR/platforms/nuttx/NuttX/nuttx" tag -l | grep -qx 'nuttx-10.3.0'; then
	git -C "$PX4_DIR/platforms/nuttx/NuttX/nuttx" tag nuttx-10.3.0
fi

echo "Installing board support ..."
rm -rf "$BOARD_DST"
mkdir -p "$(dirname "$BOARD_DST")"
cp -a "$BOARD_SRC" "$BOARD_DST"

echo "Building hsrc_f405tf_default ..."
make -C "$PX4_DIR" hsrc_f405tf_default -j"$(nproc)"

OUT="$PX4_DIR/build/hsrc_f405tf_default/hsrc_f405tf_default.bin"
echo
echo "Firmware: $OUT"
echo "Flash with: $REPO_DIR/tools/flash_px4.sh"
