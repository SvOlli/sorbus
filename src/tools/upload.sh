#!/bin/sh

readonly mydir="$(dirname "${0}")"
readonly device="/dev/ttyACM0"
readonly topdir="../../"

file="$(readlink -f "${1}")"
[ -f "${file}" ] || {
cat "${mydir}/../../build/rp2040/native_alpha.uf2" "${mydir}/../../build/rp2040/native_kernel.uf2" > "${mydir}/../../build/rp2040/native_test.uf2"
file="$(readlink -f "${mydir}/../../build/rp2040/native_test.uf2")"
}

set -ex
cd "${mydir}"
make -C "${topdir}"
picotool info "${file}" -a
picotool load "${file}" -f
picotool reboot -f
while [ ! -c "${device}" ]; do sleep 0.33; done
exec microcom -p "${device}"
