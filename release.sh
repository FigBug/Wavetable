#!/bin/bash -e
set -x

cd "$(dirname "$0")"
ROOT=$(pwd)

VER="$GITHUB_REF_NAME"

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

gh release create "$VER" -F /tmp/release_notes.txt ./Binaries\ Linux/*.zip ./Binaries\ Windows/*.zip ./Binaries\ macOS/*.zip
