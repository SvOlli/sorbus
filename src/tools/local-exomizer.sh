#!/bin/bash

readonly PACKAGE="exomizer-3.1.2"
readonly PACKAGE_BASE="${PACKAGE%-*}"
readonly URL="https://bitbucket.org/magli143/exomizer/wiki/downloads/${PACKAGE}.zip"

. "$(dirname "${0}")/local-common.sh"

# setup build environment
[ -f "$(basename "${URL}")" ] || wget "${URL}"
rm -rf "${PACKAGE}"
mkdir "${PACKAGE}"
cd "${PACKAGE}"
7z x "../$(basename "${URL}")"

# build
make -C src -j${JOBS}

# deploy manually (no "make install")
${sudo_package} install -s -m 0755 -D src/exomizer "${STOW_DIR}/${PACKAGE}/bin/exomizer"
${sudo_package} install -s -m 0755 -D src/exobasic "${STOW_DIR}/${PACKAGE}/bin/exobasic"

for file in *.txt $(find exodecrs rawdecrs -type f); do
   ${sudo_package} install "${file}" -m 0644 -D "${STOW_DIR}/${PACKAGE}/share/doc/${PACKAGE}/${file}"
done

stow_package
