#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Check pspdev
if [ -z "$PSPDEV" ]; then
    echo "Error: PSPDEV not set. Install pspdev and run: source /usr/local/pspdev/pspdev.sh"
    exit 1
fi

echo "PSPDEV: $PSPDEV"
echo "Building Space Cadet Pinball for PSP..."

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$PSPDEV/share/pspdev/psptoolchain.cmake"

# Build
make -j$(nproc)

# Verify EBOOT.PBP
if [ -f "$BUILD_DIR/EBOOT.PBP" ]; then
    echo ""
    echo "=== Build Successful ==="
    echo "EBOOT.PBP: $BUILD_DIR/EBOOT.PBP"
    echo ""
    echo "To install on PSP:"
    echo "  1. Copy EBOOT.PBP to ms0:/PSP/GAME/SpaceCadetPinball/"
    echo "  2. Copy PINBALL.DAT to ms0:/PSP/GAME/SpaceCadetPinball/"
    ls -lh "$BUILD_DIR/EBOOT.PBP"
else
    echo "Error: EBOOT.PBP not found"
    exit 1
fi
