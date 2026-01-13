#!/bin/bash
# Build Linux .deb package for Wavetable

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="${VERSION:-1.0.0}"
PLUGIN_NAME="Wavetable"
PACKAGE_NAME="wavetable"
ARCH="amd64"

# Paths
BIN_DIR="$ROOT_DIR/ci/bin"
DEB_ROOT="$SCRIPT_DIR/deb_root"
OUTPUT_DIR="$SCRIPT_DIR/output"

echo "Building Wavetable $VERSION .deb package..."

# Clean and create directories
rm -rf "$DEB_ROOT" "$OUTPUT_DIR"
mkdir -p "$DEB_ROOT"
mkdir -p "$OUTPUT_DIR"

# Create package structure
mkdir -p "$DEB_ROOT/DEBIAN"
mkdir -p "$DEB_ROOT/usr/lib/vst"
mkdir -p "$DEB_ROOT/usr/lib/vst3"
mkdir -p "$DEB_ROOT/usr/lib/lv2"
mkdir -p "$DEB_ROOT/usr/share/SocaLabs/Wavetable/Wavetables"
mkdir -p "$DEB_ROOT/usr/share/SocaLabs/Wavetable/Presets"

# Copy plugins
if [ -f "$BIN_DIR/vst/$PLUGIN_NAME.so" ]; then
    cp "$BIN_DIR/vst/$PLUGIN_NAME.so" "$DEB_ROOT/usr/lib/vst/"
fi

if [ -d "$BIN_DIR/vst3/$PLUGIN_NAME.vst3" ]; then
    cp -R "$BIN_DIR/vst3/$PLUGIN_NAME.vst3" "$DEB_ROOT/usr/lib/vst3/"
fi

if [ -d "$BIN_DIR/lv2/$PLUGIN_NAME.lv2" ]; then
    cp -R "$BIN_DIR/lv2/$PLUGIN_NAME.lv2" "$DEB_ROOT/usr/lib/lv2/"
fi

# Copy wavetables and presets
cp -R "$ROOT_DIR/plugin/Resources/WavetablesFLAC/"* "$DEB_ROOT/usr/share/SocaLabs/Wavetable/Wavetables/"
cp -R "$ROOT_DIR/plugin/Resources/Presets/"* "$DEB_ROOT/usr/share/SocaLabs/Wavetable/Presets/"

# Calculate installed size (in KB)
INSTALLED_SIZE=$(du -sk "$DEB_ROOT" | cut -f1)

# Create control file
cat > "$DEB_ROOT/DEBIAN/control" << EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: sound
Priority: optional
Architecture: $ARCH
Installed-Size: $INSTALLED_SIZE
Maintainer: SocaLabs <support@socalabs.com>
Homepage: https://socalabs.com
Description: Wavetable Synthesizer Plugin
 Wavetable is a powerful wavetable synthesizer available as
 VST2, VST3, and LV2 plugins for Linux.
 .
 Features:
  - Dual wavetable oscillators
  - Multiple filter types
  - Extensive modulation matrix
  - Built-in effects
EOF

# Create postinst script
cat > "$DEB_ROOT/DEBIAN/postinst" << 'EOF'
#!/bin/bash
# Update plugin caches if available
if command -v update-mime-database &> /dev/null; then
    update-mime-database /usr/share/mime || true
fi
exit 0
EOF
chmod 755 "$DEB_ROOT/DEBIAN/postinst"

# Set permissions
find "$DEB_ROOT" -type d -exec chmod 755 {} \;
find "$DEB_ROOT/usr" -type f -exec chmod 644 {} \;
find "$DEB_ROOT/usr/lib" -name "*.so" -exec chmod 755 {} \;

# Build the package
DEB_FILE="$OUTPUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$DEB_ROOT" "$DEB_FILE"

# Cleanup
rm -rf "$DEB_ROOT"

echo "Done! Package created: $DEB_FILE"
