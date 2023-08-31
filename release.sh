#!/bin/bash -e
set -x

cd "$(dirname "$0")"
ROOT=$(pwd)

find .

gh release create "nexus_$GITHUB_REF_NAME" -F Changelist.txt ./Binaries/*.zip
