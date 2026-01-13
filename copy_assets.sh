#!/bin/bash

# Copy Wavetable assets to system locations for local testing
# Run with: sudo ./copy_assets.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    DEST_DIR="/Library/Application Support/SocaLabs/Wavetable"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    DEST_DIR="/usr/share/SocaLabs/Wavetable"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    # Windows (Git Bash / Cygwin)
    DEST_DIR="/c/ProgramData/SocaLabs/Wavetable"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Installing assets to: $DEST_DIR"

# Create directories
mkdir -p "$DEST_DIR/Wavetables"
mkdir -p "$DEST_DIR/Presets"

# Copy wavetables (preserving subdirectory structure)
echo "Copying wavetables..."
cp -R "$SCRIPT_DIR/plugin/Resources/WavetablesFLAC/"* "$DEST_DIR/Wavetables/"

# Copy presets (preserving subdirectory structure)
echo "Copying presets..."
cp -R "$SCRIPT_DIR/plugin/Resources/Presets/"* "$DEST_DIR/Presets/"

# Set permissions so all users can read
chmod -R a+rX "$DEST_DIR"

echo "Done! Assets installed to:"
echo "  Wavetables: $DEST_DIR/Wavetables"
echo "  Presets:    $DEST_DIR/Presets"
