#!/bin/bash -e

PLUGIN="Wavetable"

# linux specific stiff
if [ $OS = "linux" ]; then
  export GDK_BACKEND=x11

  sudo apt-get update
  sudo apt-get install clang git ladspa-sdk freeglut3-dev g++ libasound2-dev libcurl4-openssl-dev libfreetype6-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev webkit2gtk-4.0 juce-tools xvfb

  Xvfb :99 &
  export DISPLAY=:99
  sleep 5
fi

# mac specific stuff
if [ $OS = "mac" ]; then
  # Create a temp keychain
  if [ -n "$GITHUB_ACTIONS" ]; then
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

ROOT=$(cd "$(dirname "$0")/.."; pwd)
cd "$ROOT"
echo "$ROOT"

cd "$ROOT/ci"
rm -Rf bin
mkdir bin

# Get the hash
cd "$ROOT/modules/juce"
HASH=`git rev-parse HEAD`
echo "Hash: $HASH"

# Get the Projucer
cd "$ROOT/ci/bin"
while true
do
  PROJUCER_URL=$(curl -s -S "https://projucer.rabien.com/get_projucer.php?hash=$HASH&os=$OS&key=$APIKEY")
  echo "Response: $PROJUCER_URL"
  if [[ $PROJUCER_URL == http* ]]; then
    curl -s -S $PROJUCER_URL -o "$ROOT/ci/bin/Projucer.zip"
    unzip Projucer.zip
    break
  fi
  sleep 15
done

# Resave jucer file
if [ "$OS" = "mac" ]; then
  "$ROOT/ci/bin/Projucer.app/Contents/MacOS/Projucer" --resave "$ROOT/plugin/$PLUGIN.jucer"
elif [ "$OS" = "linux" ]; then
  "$ROOT/ci/bin/Projucer" --resave "$ROOT/plugin/$PLUGIN.jucer"
else
  "$ROOT/ci/bin/Projucer.exe" --resave "$ROOT/plugin/$PLUGIN.jucer"
fi

# Build mac version
if [ "$OS" = "mac" ]; then
  cd "$ROOT/plugin/Builds/MacOSX"
  xcodebuild -configuration Release || exit 1

  cp -R ~/Library/Audio/Plug-Ins/VST/$PLUGIN.vst "$ROOT/ci/bin"
  cp -R ~/Library/Audio/Plug-Ins/Components/$PLUGIN.component "$ROOT/ci/bin"

  cd "$ROOT/ci/bin"
  for filename in ./*.vst; do
    codesign -s "$DEV_APP_ID" -v "$filename" --options=runtime --timestamp
  done
  for filename in ./*.component; do
    codesign -s "$DEV_APP_ID" -v "$filename" --options=runtime --timestamp
  done

  # Build notarize tool
  cd "$ROOT/modules/gin/tools/notarize"
  "$ROOT/ci/bin/Projucer.app/Contents/MacOS/Projucer" --set-global-search-path osx defaultJuceModulePath "$ROOT/modules/juce/modules" 
  "$ROOT/ci/bin/Projucer.app/Contents/MacOS/Projucer" --resave "notarize.jucer"
  cd Builds/MacOSX
  xcodebuild -configuration Release || exit 1
  cd build/Release
  cp notarize "$ROOT/ci/bin"

  # Notarize
  cd "$ROOT/ci/bin"
  zip -r ${PLUGIN}_Mac.zip $PLUGIN.vst $PLUGIN.component

  "$ROOT/ci/bin/notarize" -ns ${PLUGIN}_Mac.zip $APPLE_USER $APPLE_PASS com.figbug.$PLUGIN.vst

  rm ${PLUGIN}_Mac.zip
  xcrun stapler staple $PLUGIN.vst
  xcrun stapler staple $PLUGIN.component
  zip -r ${PLUGIN}_Mac.zip $PLUGIN.vst $PLUGIN.component

  curl -F "files=@${PLUGIN}_Mac.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi

# Build linux version
if [ "$OS" = "linux" ]; then
  cd "$ROOT/plugin/Builds/LinuxMakefile"
  make CONFIG=Release

  cd "$ROOT/plugin/Builds/LinuxMakefile"
  cp  ./build/$PLUGIN.so "$ROOT/ci/bin"

  cd "$ROOT/ci/bin"
  rm -Rf ${PLUGIN}_Linux.zip
  zip -r ${PLUGIN}_Linux.zip $PLUGIN.so

  curl -F "files=@${PLUGIN}_Linux.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi

# Build Win version
if [ "$OS" = "win" ]; then
  VS_WHERE="C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
  
  MSBUILD_EXE=$("$VS_WHERE" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe")
  echo $MSBUILD_EXE

  cd "$ROOT/plugin/Builds/VisualStudio2019"
  "$MSBUILD_EXE" "$PLUGIN.sln" "//p:VisualStudioVersion=16.0" "//m" "//t:Build" "//p:Configuration=Release64" "//p:Platform=x64" "//p:PreferredToolArchitecture=x64"
  "$MSBUILD_EXE" "$PLUGIN.sln" "//p:VisualStudioVersion=16.0" "//m" "//t:Build" "//p:Configuration=Release" "//p:PlatformTarget=x86" "//p:PreferredToolArchitecture=x64"

  cd "$ROOT/ci/bin"

  cp "$ROOT/plugin/Builds/VisualStudio2019/x64/Release64/VST/${PLUGIN}.dll" .
  cp "$ROOT/plugin/Builds/VisualStudio2019/Win32/Release/VST/${PLUGIN}_32b.dll" .

  7z a ${PLUGIN}_Win.zip ${PLUGIN}.dll ${PLUGIN}_32b.dll

  curl -F "files=@${PLUGIN}_Win.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi