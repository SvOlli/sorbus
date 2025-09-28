#!/bin/bash

set -e

cd "$(dirname "${0}")/../.."

readonly FORMAT='sorbus'

# setup workdir

#rm -rf dump
mkdir -p dump
cp src/bin/cpm/diskdefs dump/
cd dump
picotool save -r 0x10400000 0x11000000 -t bin dump.dhara -f
../build/tools/dharatool.exe dhararead dump.dhara dump.bin

for i in {0..15};do
   mkdir -p ${i}
   cpmcp -f sorbus dump.bin ${i}:* ${i}/
   rmdir ${i} 2>/dev/null || :
done

