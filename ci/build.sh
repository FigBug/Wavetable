#!/bin/bash -e
set +x

PLUGIN="Wavetable"

# linux specific stiff
if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
  sudo apt-get update
  sudo apt-get install clang git ninja-build ladspa-sdk freeglut3-dev g++ libasound2-dev libcurl4-openssl-dev libfreetype6-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev webkit2gtk-4.0 juce-tools xvfb
fi

# mac specific stuff
if [ "$(uname)" == "Darwin" ]; then
  # Create a temp keychain
  if [ -n "$GITHUB_ACTIONS" ]; then
    if [ -n "$APPLICATION" ]; then
      echo "Create a keychain"
      security create-keychain -p nr4aGPyz Keys.keychain

      echo $APPLICATION | base64 -D -o /tmp/Application.p12
      echo $INSTALLER | base64 -D -o /tmp/Installer.p12

      security import /tmp/Application.p12 -t agg -k Keys.keychain -P aym9PKWB -A -T /usr/bin/codesign
      security import /tmp/Installer.p12 -t agg -k Keys.keychain -P aym9PKWB -A -T /usr/bin/codesign

      security list-keychains -s Keys.keychain
      security default-keychain -s Keys.keychain
      security unlock-keychain -p nr4aGPyz Keys.keychain
      security set-keychain-settings -l -u -t 13600 Keys.keychain
      security set-key-partition-list -S apple-tool:,apple: -s -k nr4aGPyz Keys.keychain
    fi
    DEV_APP_ID="Developer ID Application: Roland Rabien (3FS7DJDG38)"
    DEV_INST_ID="Developer ID Installer: Roland Rabien (3FS7DJDG38)"
  fi
fi

ROOT=$(cd "$(dirname "$0")/.."; pwd)
cd "$ROOT"
echo "$ROOT"

BRANCH=${GITHUB_REF##*/}
echo "$BRANCH"

cd "$ROOT/ci"
rm -Rf bin
mkdir bin

# Build mac version
if [ "$(uname)" == "Darwin" ]; then
  cd "$ROOT"
  cmake --preset xcode
  cmake --build --preset xcode --config Release

  mkdir -p "$ROOT/ci/bin/au"
  mkdir -p "$ROOT/ci/bin/vst"
  mkdir -p "$ROOT/ci/bin/vst3"

  cp -R "$ROOT/Builds/xcode/${PLUGIN}_artefacts/Release/AU/$PLUGIN.component" "$ROOT/ci/bin/au"
  cp -R "$ROOT/Builds/xcode/${PLUGIN}_artefacts/Release/VST/$PLUGIN.vst" "$ROOT/ci/bin/vst"
  cp -R "$ROOT/Builds/xcode/${PLUGIN}_artefacts/Release/VST3/$PLUGIN.vst3" "$ROOT/ci/bin/vst3"

  cd "$ROOT/ci/bin"
  if [ -n "$APPLICATION" ]; then
    codesign -s "$DEV_APP_ID" -v vst/$PLUGIN.vst --options=runtime --timestamp --force
    codesign -s "$DEV_APP_ID" -v vst3/$PLUGIN.vst3 --options=runtime --timestamp --force
    codesign -s "$DEV_APP_ID" -v au/$PLUGIN.component --options=runtime --timestamp --force
  else
    echo "Not signing"
  fi

  # Notarize
  cd "$ROOT/ci/bin"

  if [[ -n "$APPLE_USER" ]]; then
    zip -r ${PLUGIN}_Mac.zip vst/$PLUGIN.vst vst3/$PLUGIN.vst3 au/$PLUGIN.component
    xcrun notarytool submit --verbose --apple-id "$APPLE_USER" --password "$APPLE_PASS" --team-id "3FS7DJDG38" --wait --timeout 30m ${PLUGIN}_Mac.zip

    rm ${PLUGIN}_Mac.zip
    xcrun stapler staple vst/$PLUGIN.vst
    xcrun stapler staple vst3/$PLUGIN.vst3
    xcrun stapler staple au/$PLUGIN.component
  else
    echo "Not notarizing"
  fi

  zip -r ${PLUGIN}_Mac.zip vst/$PLUGIN.vst vst3/$PLUGIN.vst3 au/$PLUGIN.component

  if [ "$BRANCH" = "release" ]; then
    curl -F "files=@${PLUGIN}_Mac.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
  fi
# Build linux version
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
  cd "$ROOT"

  cmake --preset ninja-gcc
  cmake --build --preset ninja-gcc --config Release

  mkdir -p "$ROOT/ci/bin/lv2"
  mkdir -p "$ROOT/ci/bin/vst"
  mkdir -p "$ROOT/ci/bin/vst3"

  cp -R "$ROOT/Builds/ninja-gcc/${PLUGIN}_artefacts/Release/LV2/$PLUGIN.lv2" "$ROOT/ci/bin/lv2"
  cp -R "$ROOT/Builds/ninja-gcc/${PLUGIN}_artefacts/Release/VST/lib$PLUGIN.so" "$ROOT/ci/bin/vst/$PLUGIN.so"
  cp -R "$ROOT/Builds/ninja-gcc/${PLUGIN}_artefacts/Release/VST3/$PLUGIN.vst3" "$ROOT/ci/bin/vst3"

  cd "$ROOT/ci/bin"

  # Strip debug symbols
  strip vst/$PLUGIN.so
  strip vst3/$PLUGIN.vst3/Contents/x86_64-linux/$PLUGIN.so
  strip lv2/$PLUGIN.lv2/lib$PLUGIN.so

  # Upload
  cd "$ROOT/ci/bin"
  zip -r ${PLUGIN}_Linux.zip vst/$PLUGIN.so vst3/$PLUGIN.vst3 lv2/$PLUGIN.lv2

  if [ "$BRANCH" = "release" ]; then
    curl -F "files=@${PLUGIN}_Linux.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
  fi
# Build Win version
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
  cd "$ROOT"

  cmake --preset vs
  cmake --build --preset vs --config Release

  mkdir -p "$ROOT/ci/bin/vst"
  mkdir -p "$ROOT/ci/bin/vst3"

  cp -R "$ROOT/Builds/vs/${PLUGIN}_artefacts/Release/VST/$PLUGIN.dll" "$ROOT/ci/bin/vst"
  cp -R "$ROOT/Builds/vs/${PLUGIN}_artefacts/Release/VST3/$PLUGIN.vst3" "$ROOT/ci/bin/vst3"

  cd "$ROOT/ci/bin"
  7z a ${PLUGIN}_Win.zip vst/$PLUGIN.dll vst3/$PLUGIN.vst3

  if [ "$BRANCH" = "release" ]; then
    curl -F "files=@${PLUGIN}_Win.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
  fi
fi
