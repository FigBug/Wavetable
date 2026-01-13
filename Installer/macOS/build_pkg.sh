#!/bin/bash
# Build macOS installer package for Wavetable

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="${VERSION:-1.0.0}"
PLUGIN_NAME="Wavetable"

# Paths
BIN_DIR="$ROOT_DIR/ci/bin"
PKG_ROOT="$SCRIPT_DIR/pkg_root"
OUTPUT_DIR="$SCRIPT_DIR/output"

# Signing identities (set via environment or leave empty for unsigned)
DEV_APP_ID="${DEV_APP_ID:-}"
DEV_INST_ID="${DEV_INST_ID:-}"

echo "Building Wavetable $VERSION installer..."

# Clean and create directories
rm -rf "$PKG_ROOT" "$OUTPUT_DIR"
mkdir -p "$PKG_ROOT"
mkdir -p "$OUTPUT_DIR"

# Create package root structure
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/VST"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/Components"
mkdir -p "$PKG_ROOT/Library/Application Support/SocaLabs/Wavetable/Wavetables"
mkdir -p "$PKG_ROOT/Library/Application Support/SocaLabs/Wavetable/Presets"

# Copy plugins
if [ -d "$BIN_DIR/vst/$PLUGIN_NAME.vst" ]; then
    cp -R "$BIN_DIR/vst/$PLUGIN_NAME.vst" "$PKG_ROOT/Library/Audio/Plug-Ins/VST/"
fi

if [ -d "$BIN_DIR/vst3/$PLUGIN_NAME.vst3" ]; then
    cp -R "$BIN_DIR/vst3/$PLUGIN_NAME.vst3" "$PKG_ROOT/Library/Audio/Plug-Ins/VST3/"
fi

if [ -d "$BIN_DIR/au/$PLUGIN_NAME.component" ]; then
    cp -R "$BIN_DIR/au/$PLUGIN_NAME.component" "$PKG_ROOT/Library/Audio/Plug-Ins/Components/"
fi

# Copy wavetables and presets
cp -R "$ROOT_DIR/plugin/Resources/WavetablesFLAC/"* "$PKG_ROOT/Library/Application Support/SocaLabs/Wavetable/Wavetables/"
cp -R "$ROOT_DIR/plugin/Resources/Presets/"* "$PKG_ROOT/Library/Application Support/SocaLabs/Wavetable/Presets/"

# Set permissions
chmod -R 755 "$PKG_ROOT/Library/Audio"
chmod -R 755 "$PKG_ROOT/Library/Application Support"

# Build component package
pkgbuild --root "$PKG_ROOT" \
    --identifier "com.socalabs.wavetable.pkg" \
    --version "$VERSION" \
    --install-location "/" \
    "$OUTPUT_DIR/Wavetable-component.pkg"

# Create distribution.xml
cat > "$OUTPUT_DIR/distribution.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>Wavetable $VERSION</title>
    <organization>com.socalabs</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="true" rootVolumeOnly="true" />
    <welcome file="welcome.txt" mime-type="text/plain" />
    <choices-outline>
        <line choice="default">
            <line choice="com.socalabs.wavetable.pkg"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="com.socalabs.wavetable.pkg" visible="false">
        <pkg-ref id="com.socalabs.wavetable.pkg"/>
    </choice>
    <pkg-ref id="com.socalabs.wavetable.pkg" version="$VERSION" onConclusion="none">Wavetable-component.pkg</pkg-ref>
</installer-gui-script>
EOF

# Create welcome text
cat > "$OUTPUT_DIR/welcome.txt" << EOF
Welcome to the Wavetable $VERSION installer.

This will install:
- Wavetable VST2 plugin
- Wavetable VST3 plugin
- Wavetable AU plugin
- Factory wavetables
- Factory presets

Click Continue to proceed with the installation.
EOF

# Build product archive
UNSIGNED_PKG="$OUTPUT_DIR/${PLUGIN_NAME}_${VERSION}_Mac_unsigned.pkg"
SIGNED_PKG="$OUTPUT_DIR/${PLUGIN_NAME}_${VERSION}_Mac.pkg"

productbuild --distribution "$OUTPUT_DIR/distribution.xml" \
    --package-path "$OUTPUT_DIR" \
    --resources "$OUTPUT_DIR" \
    "$UNSIGNED_PKG"

# Sign if identity is available
if [ -n "$DEV_INST_ID" ]; then
    echo "Signing installer..."
    productsign --sign "$DEV_INST_ID" "$UNSIGNED_PKG" "$SIGNED_PKG"
    rm "$UNSIGNED_PKG"
    echo "Signed installer: $SIGNED_PKG"
else
    mv "$UNSIGNED_PKG" "$SIGNED_PKG"
    echo "Unsigned installer: $SIGNED_PKG"
fi

# Cleanup
rm -rf "$PKG_ROOT"
rm -f "$OUTPUT_DIR/Wavetable-component.pkg"
rm -f "$OUTPUT_DIR/distribution.xml"
rm -f "$OUTPUT_DIR/welcome.txt"

echo "Done! Installer created: $SIGNED_PKG"
