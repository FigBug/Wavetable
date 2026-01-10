#!/bin/bash -e

cd "$(dirname "$0")"
ROOT=$(pwd)

cd $ROOT

VER=`cat VERSION`

# Check that the version exists in the Changelist.txt
if ! grep -q "^${VER}:*$" Changelist.txt; then
    echo "Error: Version $VER not found in Changelist.txt"
    exit 1
fi

echo "Tagging [$VER]"
git tag "$VER" && git push origin "$VER"
