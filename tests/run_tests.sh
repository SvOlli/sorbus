#!/bin/bash

set -e

make

if [[ ! -f ../build/rp2040/jam_alpha_picotool.uf2 ]]; then
  echo "Build Sorbus firmware first"
  exit 1
fi

exec ./test_runner
