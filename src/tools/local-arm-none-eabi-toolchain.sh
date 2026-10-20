#!/bin/bash

readonly BASE_URL="https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain"
readonly VERSION="15.3.rel2"
readonly ARCH="$(uname -m)"
readonly ARCHIVE_NAME="arm-gnu-toolchain-${VERSION}-${ARCH}-arm-none-eabi.tar.xz"
readonly PACKAGE="${ARCHIVE_NAME%.tar.xz}"
readonly PACKAGE_BASE="arm-gnu-toolchain"

. "$(dirname "${0}")/local-common.sh"

#https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/15.3.rel2/arm-gnu-toolchain-15.3.rel2-x86_64-arm-none-eabi.tar.xz
#https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/15.3.rel2/arm-gnu-toolchain-15.3.rel2-aarch64-arm-none-eabi.tar.xz

if [ ! -f "${ARCHIVE_NAME}" ]; then
   wget "${BASE_URL}/${VERSION}/${ARCHIVE_NAME}"
fi

cd "${STOW_DIR}"
${sudo_package} tar -xf "${BUILD_DIR}/${ARCHIVE_NAME}"
${sudo_package} mkdir -p "${PACKAGE}/share/doc/${PACKAGE}"
${sudo_package} mv "${PACKAGE}"/*.txt "${PACKAGE}/share/doc/${PACKAGE}"
${sudo_package} chown -R $(stat -c "%u:%g" "${STOW_DIR}") "${STOW_DIR}/${PACKAGE}"

stow_package
