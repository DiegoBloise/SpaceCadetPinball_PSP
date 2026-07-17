#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Auto-detect PSPDEV if not set
if [ -z "$PSPDEV" ]; then
    for candidate in /opt/pspdev /usr/local/pspdev; do
        if [ -d "$candidate/bin" ]; then
            export PSPDEV="$candidate"
            break
        fi
    done
    if [ -z "$PSPDEV" ]; then
        echo "Error: PSPDEV not found. Install pspdev or set PSPDEV manually."
        exit 1
    fi
fi

export PATH="$PSPDEV/bin:$PATH"

echo "PSPDEV: $PSPDEV"
echo "Building Space Cadet Pinball for PSP..."

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$PSPDEV/psp/share/pspdev.cmake"

# Build
make -j$(nproc)

# Verify EBOOT.PBP (created by create_pbp_file in bin/)
EBOOT_PATH="$SCRIPT_DIR/bin/EBOOT.PBP"
if [ -f "$EBOOT_PATH" ]; then
    echo ""
    echo "=== Build Successful ==="
    echo "EBOOT.PBP: $EBOOT_PATH"
    echo ""
    echo "To install on PSP:"
    echo "  1. Copy EBOOT.PBP to ms0:/PSP/GAME/SpaceCadetPinball/"
    echo "  2. Copy PINBALL.DAT, sound fx files and PINBALL.WAV to ms0:/PSP/GAME/SpaceCadetPinball/"
    ls -lh "$EBOOT_PATH"
else
    echo "Error: EBOOT.PBP not found at $EBOOT_PATH"
    exit 1
fi
