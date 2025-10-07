#!/bin/sh

readonly mydir="$(readlink -f "$(dirname "${0}")")"
readonly device="/dev/ttyACM0"
readonly topdir="../../"

if [ -n "${1}" ]; then
   file="$(readlink -f "${1}")"
fi

set -ex
cd "${mydir}"
make -C "${topdir}"
if [ -z "${file}" ]; then
   cat "${mydir}/../../build/rp2040/jam_alpha.uf2" "${mydir}/../../build/rp2040/jam_rom.uf2" > "${mydir}/../../build/rp2040/jam_test.uf2"
   file="$(readlink -f "${mydir}/../../build/rp2040/jam_test.uf2")"
fi
picotool info "${file}" -a || :
picotool load "${file}" -f
picotool reboot -f || :
while [ ! -c "${device}" ]; do sleep 0.33; done; sleep 0.2
exec microcom -p "${device}"
