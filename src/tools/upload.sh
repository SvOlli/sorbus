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
   cat "${mydir}/../../build/rp2040/native_alpha.uf2" "${mydir}/../../build/rp2040/native_kernel.uf2" > "${mydir}/../../build/rp2040/native_test.uf2"
   file="$(readlink -f "${mydir}/../../build/rp2040/native_test.uf2")"
fi
picotool info "${file}" -a || :
picotool load "${file}" -f
picotool reboot -f
while [ ! -c "${device}" ]; do sleep 0.33; done
exec microcom -p "${device}"
