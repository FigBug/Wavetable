#!/bin/bash -e

cd "$(dirname "$0")"
ROOT=$(pwd)

cd $ROOT

VER=`cat VERSION`
echo "Tagging [$VER]"
git tag "$VER" && git push origin "$VER"
