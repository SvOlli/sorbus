#!/bin/sh

readonly mydir="$(dirname "${0}")"
readonly device="/dev/ttyACM0"
readonly topdir="../../"

file="$(readlink -f "${1}")"
[ -f "${file}" ] || file="$(readlink -f "${mydir}/../../build/rp2040/mcp.elf")"

set -ex
cd "${mydir}"
make -C "${topdir}"
picotool load "${file}" -f
while [ ! -c "${device}" ]; do sleep 0.33; done
exec microcom -p "${device}"
