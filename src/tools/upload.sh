#!/bin/sh

file="../../build/rp2040/mcp.elf"

[ -f "${1}" ] && file="${1}"

set -ex
make -C ../../build
lsusb | grep '2e8a:000[3a]' | tr ':' ' ' | while read dummy bus dummy device dummy; do
   picotool load "${file}" --bus ${bus} --address ${device} -f
done
while [ ! -c /dev/ttyACM0 ]; do sleep 0.33; done
exec microcom -p /dev/ttyACM0
