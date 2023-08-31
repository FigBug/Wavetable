#!/bin/bash -e
set -x

cd "$(dirname "$0")"
cd ..
ROOT=$(pwd)

gh release create "nexus_$GITHUB_REF_NAME" -F Changelist.txt ../Binaries/*.zip
