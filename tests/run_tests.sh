#!/bin/bash

set -e errexit

if [[ ! -d rp2040js ]]; then
  git clone --depth 1 https://github.com/c1570/rp2040js.git
  cd rp2040js/
  npm install
  npx tsc demo/bootrom.ts --skipLibCheck
  cd ..
fi

make
npm install

if [[ ! -f ../build/rp2040/jam_alpha_picotool.uf2 ]]; then
  echo "Build Sorbus firmware first"
  exit 1
fi

if egrep -q "pico_enable_stdio_usb.*1\)" ../src/rp2040/CMakeLists.txt ; then
  echo -e "\n*** Best enable RP2 serial stdio for testing ***"
  sleep 1
fi

npm run test
