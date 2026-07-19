#!/bin/bash -e
set -x

cd "$(dirname "$0")"
ROOT=$(pwd)

TAG="$GITHUB_REF_NAME"
# Tags are pushed with a leading "v" (see tag.sh); strip for the changelog
# lookup and the upload.php version field, but use TAG for gh release create.
VER="${TAG#v}"

# Extract the changelog section for the current version
# Matches version with optional colon, captures until the next version line or EOF
NOTES=$(awk -v ver="$VER" '
    BEGIN { found=0; printing=0; pattern="^"ver":?$" }
    $0 ~ pattern { found=1; printing=1; next }
    printing && /^[0-9]+\.[0-9]+\.[0-9]+:?$/ { printing=0 }
    printing { print }
    END { if (!found) exit 1 }
' ./Changelist.txt)

if [ $? -ne 0 ] || [ -z "$NOTES" ]; then
    echo "Error: Version $VER not found in Changelist.txt or has no content"
    exit 1
fi

echo "$NOTES" > /tmp/release_notes.txt

# --- Debug symbols -> crash server -------------------------------------------
# Uploaded here (not the build job) so a failed upload fails the release. The
# server stores symbols write-once per (plugin, platform, version): a re-tagged
# or rebuilt version whose symbols already exist returns 409 and fails the
# release loudly — delete the stale symbols in the crash site, then re-run.
CRASH_BASE="https://crashreports.rabiensoftware.com"
upload_symbols () { # $1 platform, $2 zip
  if [ ! -f "$2" ]; then echo "Error: expected symbols $2 not found"; exit 1; fi
  echo "Uploading $1 symbols for $VER"
  curl -sS --fail-with-body -H "X-API-Key: $SYMBOL_API_KEY" \
    -F "platform=$1" -F "version=$VER" -F "files[]=@$2" \
    "$CRASH_BASE/symbols/"
  echo
}
upload_symbols mac "./Binaries macOS/Symbols_Mac.zip"
upload_symbols win "./Binaries Windows/Symbols_Win.zip"

ASSETS=(
  "./Binaries Linux"/*.deb
  "./Binaries Windows"/*.exe
  "./Binaries macOS"/*.pkg
)
if [ -f "./Binaries macOS/Symbols_Mac.zip" ]; then
  ASSETS+=("./Binaries macOS/Symbols_Mac.zip")
fi

gh release create "$TAG" --title "$TAG" -F /tmp/release_notes.txt "${ASSETS[@]}"

PLUGIN=wavetable
for f in "./Binaries Linux"/*.deb \
         "./Binaries Windows"/*.exe \
         "./Binaries macOS"/*.pkg; do
  curl -sS --fail-with-body -F "files=@${f}" \
          -F "plugin=${PLUGIN}" \
          -F "version=${VER}" \
          -F "changelog=${NOTES}" \
          "https://socalabs.com/files/upload.php?key=$APIKEY"
done
