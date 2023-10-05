#!/bin/bash -e
set +x

PLUGIN="Wavetable"

ROOT=$(cd "$(dirname "$0")/.."; pwd)
cd "$ROOT"
echo "$ROOT"

# Upload mac version
if [ "$(uname)" == "Darwin" ]; then
  cd "$ROOT/ci/bin"
  curl -F "files=@${PLUGIN}_Mac.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi

# Upload linux version
if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
  cd "$ROOT/ci/bin"
  curl -F "files=@${PLUGIN}_Linux.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi

# Upload Win version
if [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
  cd "$ROOT/ci/bin"
  curl -F "files=@${PLUGIN}_Win.zip" "https://socalabs.com/files/set.php?key=$APIKEY"
fi
