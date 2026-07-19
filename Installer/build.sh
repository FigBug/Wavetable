#!/bin/bash -e
set -x

#
# Resolve repo root
#
cd "$(dirname "$0")"
cd ..
PROJECT_ROOT=$(pwd)
PLUGIN="Wavetable"
VENDOR="SocaLabs"
BUNDLE_BASE="com.socalabs.wavetable"

if [ "$(uname)" = "Darwin" ]; then
  PLATFORM="macOS"
elif [ "$(expr substr "$(uname -s)" 1 5)" = "Linux" ]; then
  PLATFORM="linux"
else
  PLATFORM="win"
fi

VERSION=$(cat "$PROJECT_ROOT/VERSION")

#
# Crash reporting: bundle the latest CrashReporter app + this plugin's
# registration JSON. Debug symbols are zipped into bin/ and uploaded later by
# the release job (release.sh), never from this build.
# The CrashReporter download is a public distribution channel (no key needed).
#
CRASH_BASE="https://crashreports.rabiensoftware.com"

# Native (non-MSYS) curl on Windows can't read Git-Bash paths like /d/a/...,
# so translate to a Windows path there. No-op on macOS/Linux.
curl_path () {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else printf '%s' "$1"; fi
}

# Download the latest CrashReporter build for a platform. Non-fatal: if none is
# published yet we still ship the registration JSON so crashes register.
fetch_reporter () { # $1 platform, $2 output file
  if curl -fsSL "$CRASH_BASE/reporter/latest/?platform=$1" -o "$(curl_path "$2")"; then
    return 0
  fi
  echo "WARNING: could not fetch CrashReporter for $1 (none published yet?)"
  return 1
}

#
# Reset staging
#
rm -Rf "$PROJECT_ROOT/Installer/$PLATFORM/bin"
mkdir -p "$PROJECT_ROOT/Installer/$PLATFORM/bin"
rm -Rf "$PROJECT_ROOT/bin"
mkdir -p "$PROJECT_ROOT/bin"

#
# Flatten the nested Presets/<category>/*.xml tree into a single flat directory.
# gin's loadDirectory() doesn't recurse, and the legacy BinaryData flow stored
# presets flat in the user directory — keeping flat matches what users see.
#
FLAT_PRESETS="$PROJECT_ROOT/Installer/_flat_presets"
rm -Rf "$FLAT_PRESETS"
mkdir -p "$FLAT_PRESETS"
find "$PROJECT_ROOT/plugin/Resources/Presets" -name "*.xml" -type f -exec cp {} "$FLAT_PRESETS/" \;

############################################################
# macOS — pkgbuild + productbuild
############################################################
if [ "$PLATFORM" = "macOS" ]; then
  TEAM_ID="${TEAM_ID:-3FS7DJDG38}"
  DEV_APP_ID="${DEV_APP_ID:-Developer ID Application: Roland Rabien (${TEAM_ID})}"
  DEV_INST_ID="${DEV_INST_ID:-Developer ID Installer: Roland Rabien (${TEAM_ID})}"

  #
  # Bootstrap a temp keychain from base64-encoded p12 secrets when running in CI.
  # APPLICATION = base64 of Developer ID Application .p12
  # INSTALLER   = base64 of Developer ID Installer   .p12
  #
  if [ -n "${APPLICATION:-}" ] && [ -n "${INSTALLER:-}" ]; then
    KC_PASS="${KEYCHAIN_PASSWORD:-nr4aGPyz}"
    P12_PASS="${P12_PASSWORD:-aym9PKWB}"

    security create-keychain -p "$KC_PASS" Keys.keychain || true

    echo "$APPLICATION" | base64 -D -o /tmp/Application.p12
    echo "$INSTALLER"   | base64 -D -o /tmp/Installer.p12

    security import /tmp/Application.p12 -t agg -k Keys.keychain -P "$P12_PASS" -A -T /usr/bin/codesign
    security import /tmp/Installer.p12   -t agg -k Keys.keychain -P "$P12_PASS" -A -T /usr/bin/productsign

    security list-keychains -s Keys.keychain
    security default-keychain -s Keys.keychain
    security unlock-keychain -p "$KC_PASS" Keys.keychain
    security set-keychain-settings -l -u -t 13600 Keys.keychain
    security set-key-partition-list -S apple-tool:,apple: -s -k "$KC_PASS" Keys.keychain
  fi

  ART_DIR="$PROJECT_ROOT/Builds/xcode/${PLUGIN}_artefacts/Release"

  cd "$PROJECT_ROOT"
  cmake --preset xcode
  cmake --build --preset xcode --config Release

  STAGE="$PROJECT_ROOT/Installer/macOS/bin/stage"
  PKG_DIR="$PROJECT_ROOT/Installer/macOS/bin/pkgs"
  rm -Rf "$STAGE" "$PKG_DIR"
  mkdir -p "$STAGE/vst" "$STAGE/vst3" "$STAGE/au" "$STAGE/clap"
  mkdir -p "$STAGE/resources/Library/Audio/Presets/$VENDOR/$PLUGIN"
  mkdir -p "$PKG_DIR"

  cp -RL "$ART_DIR/VST/$PLUGIN.vst"      "$STAGE/vst/"
  cp -RL "$ART_DIR/VST3/$PLUGIN.vst3"    "$STAGE/vst3/"
  cp -RL "$ART_DIR/AU/$PLUGIN.component" "$STAGE/au/"
  cp -RL "$ART_DIR/CLAP/$PLUGIN.clap"    "$STAGE/clap/"

  # Resources component: factory Wavetables and Presets (presets flattened above)
  mkdir -p "$STAGE/resources/Library/Audio/Presets/$VENDOR/$PLUGIN/Presets"
  cp "$FLAT_PRESETS/"*.xml "$STAGE/resources/Library/Audio/Presets/$VENDOR/$PLUGIN/Presets/"
  cp -R "$PROJECT_ROOT/plugin/Resources/WavetablesFLAC" "$STAGE/resources/Library/Audio/Presets/$VENDOR/$PLUGIN/Wavetables"
  find "$STAGE/resources" -name ".DS_Store" -delete

  # Strip symbols from the shipped binaries so end-user crash logs are NOT
  # symbolicated locally by macOS — the server symbolicates them from the dSYMs
  # (built below from the unstripped products in $ART_DIR). strip preserves the
  # Mach-O UUID, so the dSYM still matches. Before codesign.
  strip -x "$STAGE/vst/$PLUGIN.vst/Contents/MacOS/$PLUGIN"
  strip -x "$STAGE/vst3/$PLUGIN.vst3/Contents/MacOS/$PLUGIN"
  strip -x "$STAGE/au/$PLUGIN.component/Contents/MacOS/$PLUGIN"
  strip -x "$STAGE/clap/$PLUGIN.clap/Contents/MacOS/$PLUGIN"

  if [ -n "${APPLICATION:-}" ]; then
    codesign -s "$DEV_APP_ID" --options=runtime --timestamp --force -v "$STAGE/vst/$PLUGIN.vst"
    codesign -s "$DEV_APP_ID" --options=runtime --timestamp --force -v "$STAGE/vst3/$PLUGIN.vst3"
    codesign -s "$DEV_APP_ID" --options=runtime --timestamp --force -v "$STAGE/au/$PLUGIN.component"
    codesign -s "$DEV_APP_ID" --options=runtime --timestamp --force -v "$STAGE/clap/$PLUGIN.clap"
  else
    echo "Skipping codesign — APPLICATION secret not set"
  fi

  # Component packages
  pkgbuild --root "$STAGE/vst" \
           --install-location "/Library/Audio/Plug-Ins/VST" \
           --identifier "${BUNDLE_BASE}.vst.pkg" \
           --version "$VERSION" \
           "$PKG_DIR/vst.pkg"

  pkgbuild --root "$STAGE/vst3" \
           --install-location "/Library/Audio/Plug-Ins/VST3" \
           --identifier "${BUNDLE_BASE}.vst3.pkg" \
           --version "$VERSION" \
           "$PKG_DIR/vst3.pkg"

  pkgbuild --root "$STAGE/au" \
           --install-location "/Library/Audio/Plug-Ins/Components" \
           --identifier "${BUNDLE_BASE}.au.pkg" \
           --version "$VERSION" \
           "$PKG_DIR/au.pkg"

  pkgbuild --root "$STAGE/clap" \
           --install-location "/Library/Audio/Plug-Ins/CLAP" \
           --identifier "${BUNDLE_BASE}.clap.pkg" \
           --version "$VERSION" \
           "$PKG_DIR/clap.pkg"

  pkgbuild --root "$STAGE/resources" \
           --install-location "/" \
           --identifier "${BUNDLE_BASE}.resources.pkg" \
           --version "$VERSION" \
           --scripts "$PROJECT_ROOT/Installer/macOS/scripts" \
           "$PKG_DIR/resources.pkg"

  # CrashReporter component: latest signed CrashReporter.app + this plugin's
  # registration JSON. Staged under .incoming and promoted by the postinstall
  # only if strictly newer (shared component, never downgraded).
  REP_STAGE="$PROJECT_ROOT/Installer/macOS/bin/reporter"
  REP_ROOT="$REP_STAGE/Library/Application Support/Rabien Software/Crash Reporter"
  rm -Rf "$REP_STAGE"
  mkdir -p "$REP_ROOT/Plugins" "$REP_ROOT/.incoming"
  if fetch_reporter mac "$REP_STAGE/CrashReporter_Mac.zip"; then
    ( cd "$REP_STAGE" && unzip -qo CrashReporter_Mac.zip && rm CrashReporter_Mac.zip )
    mv "$REP_STAGE/CrashReporter.app" "$REP_ROOT/.incoming/"
  fi
  cp "$PROJECT_ROOT/Installer/crashreporter.json" "$REP_ROOT/Plugins/wavetable.json"
  find "$REP_STAGE" -name ".DS_Store" -delete

  if [ -n "${APPLICATION:-}" ] && [ -d "$REP_ROOT/.incoming/CrashReporter.app" ]; then
    codesign -s "$DEV_APP_ID" --options=runtime --timestamp --force -v "$REP_ROOT/.incoming/CrashReporter.app"
  fi

  chmod +x "$PROJECT_ROOT/Installer/macOS/reporter-scripts/postinstall"

  # Disable bundle relocation so the installer installs to our staged path
  # instead of redirecting to a CrashReporter.app found elsewhere via Spotlight.
  COMP_PLIST="$PROJECT_ROOT/Installer/macOS/bin/reporter-component.plist"
  pkgbuild --analyze --root "$REP_STAGE" "$COMP_PLIST"
  /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$COMP_PLIST" 2>/dev/null || true

  pkgbuild --root "$REP_STAGE" --install-location "/" \
           --identifier "${BUNDLE_BASE}.crashreporter.pkg" --version "$VERSION" \
           --component-plist "$COMP_PLIST" \
           --scripts "$PROJECT_ROOT/Installer/macOS/reporter-scripts" \
           "$PKG_DIR/reporter.pkg"

  # productbuild — combine into one signed installer
  cp "$PROJECT_ROOT/Installer/EULA.rtf"           "$PKG_DIR/EULA.rtf"
  cp "$PROJECT_ROOT/Installer/macOS/welcome.txt"  "$PKG_DIR/welcome.txt"

  PKG_OUT="$PROJECT_ROOT/Installer/macOS/bin/${PLUGIN}.pkg"

  productbuild --distribution "$PROJECT_ROOT/Installer/macOS/distribution.xml" \
               --package-path "$PKG_DIR" \
               --resources "$PKG_DIR" \
               --version "$VERSION" \
               "$PKG_OUT.unsigned"

  if [ -n "${INSTALLER:-}" ]; then
    productsign --sign "$DEV_INST_ID" "$PKG_OUT.unsigned" "$PKG_OUT"
    rm "$PKG_OUT.unsigned"
  else
    echo "Skipping productsign — INSTALLER secret not set"
    mv "$PKG_OUT.unsigned" "$PKG_OUT"
  fi

  if [ -n "${APPLE_USER:-}" ] && [ -n "${APPLE_PASS:-}" ]; then
    SUBMISSION_OUTPUT=$(xcrun notarytool submit --verbose --apple-id "$APPLE_USER" --password "$APPLE_PASS" --team-id "$TEAM_ID" --wait --timeout 30m "$PKG_OUT" 2>&1) || NOTARY_FAILED=1
    echo "$SUBMISSION_OUTPUT"
    SUBMISSION_ID=$(echo "$SUBMISSION_OUTPUT" | awk "/^  id:/ { print \$2; exit }")
    if [ "${NOTARY_FAILED:-0}" = "1" ] && [ -n "$SUBMISSION_ID" ]; then
      echo "Notarization failed — fetching log for $SUBMISSION_ID"
      xcrun notarytool log "$SUBMISSION_ID" --apple-id "$APPLE_USER" --password "$APPLE_PASS" --team-id "$TEAM_ID" || true
      exit 1
    fi
    xcrun stapler staple "$PKG_OUT"
  else
    echo "Skipping notarization — APPLE_USER / APPLE_PASS not set"
  fi

  cp "$PKG_OUT" "$PROJECT_ROOT/bin/"

  # Symbols zip. Some Xcode/CMake combos don't emit .dSYMs next to the product,
  # so generate any that are missing straight from the built binaries.
  for pair in "VST:$PLUGIN.vst" "VST3:$PLUGIN.vst3" "AU:$PLUGIN.component" "CLAP:$PLUGIN.clap"; do
    d="${pair%%:*}"; b="${pair#*:}"
    dsym="$ART_DIR/$d/$b.dSYM"
    bin="$ART_DIR/$d/$b/Contents/MacOS/$PLUGIN"
    if [ ! -d "$dsym" ] && [ -f "$bin" ]; then
      echo "Generating dSYM for $d/$b"
      dsymutil "$bin" -o "$dsym" || true
    fi
  done

  cd "$ART_DIR"
  zip -r "$PROJECT_ROOT/bin/Symbols_Mac.zip" \
    AU/$PLUGIN.component.dSYM \
    VST/$PLUGIN.vst.dSYM \
    VST3/$PLUGIN.vst3.dSYM \
    CLAP/$PLUGIN.clap.dSYM 2>/dev/null || true

############################################################
# Linux — cpack DEB (VST + VST3 + LV2 + CLAP + Resources)
############################################################
elif [ "$PLATFORM" = "linux" ]; then
  cd "$PROJECT_ROOT"
  cmake --preset ninja-gcc
  cmake --build --preset ninja-gcc --config Release

  cd "$PROJECT_ROOT/Builds/ninja-gcc"
  cpack -G DEB -C Release

  cp "$PROJECT_ROOT/Builds/ninja-gcc/"*.deb "$PROJECT_ROOT/bin/"

############################################################
# Windows — Inno Setup + Azure Trusted Signing
############################################################
else
  cd "$PROJECT_ROOT"
  cmake --preset vs
  cmake --build --preset vs --config Release

  ART_DIR="$PROJECT_ROOT/Builds/vs/${PLUGIN}_artefacts/Release"
  STAGE="$PROJECT_ROOT/Installer/win/bin"

  rm -Rf "$STAGE"
  mkdir -p "$STAGE/VST" "$STAGE/VST3" "$STAGE/CLAP"
  cp -R "$ART_DIR/VST/$PLUGIN.dll"   "$STAGE/VST/"
  cp -R "$ART_DIR/VST3/$PLUGIN.vst3" "$STAGE/VST3/"
  cp -R "$ART_DIR/CLAP/$PLUGIN.clap" "$STAGE/CLAP/"

  # CrashReporter: latest signed build + this plugin's registration JSON.
  REP_DIR="$STAGE/CrashReporter"
  rm -Rf "$REP_DIR"; mkdir -p "$REP_DIR"
  if fetch_reporter win "$REP_DIR/CrashReporter_Win.zip"; then
    ( cd "$REP_DIR" && 7z x -y CrashReporter_Win.zip >/dev/null && rm CrashReporter_Win.zip )
  fi
  cp "$PROJECT_ROOT/Installer/crashreporter.json" "$REP_DIR/wavetable.json"

  #
  # Sign binaries via Microsoft Trusted Signing.
  # Required env: AZURE_TENANT_ID, AZURE_CLIENT_ID, AZURE_CLIENT_SECRET.
  # Cert is referenced via Installer/win/metadata.json.
  #
  uuid_re='^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$'
  WIN_SIGN=0
  if [ -n "${AZURE_TENANT_ID:-}" ] || [ -n "${AZURE_CLIENT_ID:-}" ] || [ -n "${AZURE_CLIENT_SECRET:-}" ]; then
    if   [[ ! "${AZURE_TENANT_ID:-}" =~ $uuid_re ]]; then
      echo "ERROR: AZURE_TENANT_ID is not a valid GUID — set the secret on the repo or unset all three to skip signing."; exit 1
    elif [[ ! "${AZURE_CLIENT_ID:-}" =~ $uuid_re ]]; then
      echo "ERROR: AZURE_CLIENT_ID is not a valid GUID — set the secret on the repo or unset all three to skip signing."; exit 1
    elif [ -z "${AZURE_CLIENT_SECRET:-}" ]; then
      echo "ERROR: AZURE_CLIENT_SECRET is empty — set the secret on the repo or unset all three to skip signing."; exit 1
    else
      WIN_SIGN=1
    fi
  fi

  if [ "$WIN_SIGN" = "1" ]; then
    # Locate signtool — the version dir under Windows Kits varies by runner image.
    SIGNTOOL=$(ls -1 "/c/Program Files (x86)/Windows Kits/10/bin/"*/x64/signtool.exe 2>/dev/null | sort | tail -1)
    if [ -z "$SIGNTOOL" ]; then
      echo "signtool.exe not found in Windows Kits"; exit 1
    fi

    TOOLS_DIR="$STAGE/_signingtools"
    mkdir -p "$TOOLS_DIR"
    nuget install Microsoft.Trusted.Signing.Client -Version 1.0.86 \
                  -OutputDirectory "$TOOLS_DIR" -ExcludeVersion -NonInteractive
    DLIB="$TOOLS_DIR/Microsoft.Trusted.Signing.Client/bin/x64/Azure.CodeSigning.Dlib.dll"
    METADATA="$PROJECT_ROOT/Installer/win/metadata.json"

    sign_file () {
      "$SIGNTOOL" sign -v -fd SHA256 \
        -tr "http://timestamp.acs.microsoft.com" -td SHA256 \
        -dlib "$DLIB" -dmdf "$METADATA" "$1"
    }

    sign_file "$STAGE/VST/$PLUGIN.dll"
    sign_file "$STAGE/VST3/$PLUGIN.vst3/Contents/x86_64-win/$PLUGIN.vst3"
    sign_file "$STAGE/CLAP/$PLUGIN.clap"
  else
    echo "Skipping Windows binary signing — Azure secrets not set"
  fi

  # Build installer .exe
  cd "$PROJECT_ROOT/Installer/win"
  ISCC="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
  if [ ! -f "$ISCC" ]; then
    ISCC="/c/Program Files/Inno Setup 6/ISCC.exe"
  fi
  "$ISCC" "$PROJECT_ROOT/Installer/win/${PLUGIN}.iss"

  EXE_OUT="$PROJECT_ROOT/Installer/win/bin/${PLUGIN}.exe"

  # Sign installer
  if [ "$WIN_SIGN" = "1" ]; then
    sign_file "$EXE_OUT"
  fi

  cp "$EXE_OUT" "$PROJECT_ROOT/bin/"

  # Symbols zip — PDBs for crash symbolication.
  SYM_DIR="$STAGE/symbols"
  rm -Rf "$SYM_DIR"; mkdir -p "$SYM_DIR/VST" "$SYM_DIR/VST3" "$SYM_DIR/CLAP"
  cp "$ART_DIR/VST/$PLUGIN.pdb"  "$SYM_DIR/VST/"  2>/dev/null || true
  cp "$ART_DIR/VST3/$PLUGIN.pdb" "$SYM_DIR/VST3/" 2>/dev/null || true
  cp "$ART_DIR/CLAP/$PLUGIN.pdb" "$SYM_DIR/CLAP/" 2>/dev/null || true
  ( cd "$SYM_DIR" && 7z a "$PROJECT_ROOT/bin/Symbols_Win.zip" VST VST3 CLAP )
fi
