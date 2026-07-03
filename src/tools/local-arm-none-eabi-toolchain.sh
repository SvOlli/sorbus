#!/bin/bash

readonly BASE_URL="https://developer.arm.com/-/media/files/downloads/gnu"
readonly VERSION="12.2.rel1"
readonly ARCH="$(uname -m)"
readonly ARCHIVE_NAME="arm-gnu-toolchain-${VERSION}-${ARCH}-arm-none-eabi.tar.xz"
readonly PACKAGE="${ARCHIVE_NAME%.tar.xz}"
readonly PACKAGE_BASE="arm-gnu-toolchain"

. "$(dirname "${0}")/local-common.sh"

#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz?rev=7bd049b7a3034e64885fa1a71c12f91d&revision=7bd049b7-a303-4e64-885f-a1a71c12f91d&hash=B698685C08AD15D3FD8B1BF2DB28D282
#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi.tar.xz.sha256asc?rev=abe4517f445d4540b9eb8beec2ae59f5&revision=abe4517f-445d-4540-b9eb-8beec2ae59f5&hash=530BFDDA320CDD8716E1A65301302FB1

#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi.tar.xz?rev=04bfc790b30b477fab2621438ab231a7&revision=04bfc790-b30b-477f-ab26-21438ab231a7&hash=AF19328AF490643F38711DDA862663DE
#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi.tar.xz.sha256asc?rev=23259bb385d64dec8a841f1a0f60615a&revision=23259bb3-85d6-4dec-8a84-1f1a0f60615a&hash=2EC2F2C4F55BA8F934044A8C804103C7

#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/srcrel/arm-gnu-toolchain-src-snapshot-12.2.rel1.tar.xz?rev=fdf97c204acd47f8a7ddd122c465c424&revision=fdf97c20-4acd-47f8-a7dd-d122c465c424&hash=612FE59350AB0C87BDEFD66DBCFE5B2C
#https://developer.arm.com/-/media/files/downloads/gnu/12.2.rel1/srcrel/arm-gnu-toolchain-src-snapshot-12.2.rel1-manifest.txt?rev=2cace8b49ed242a7b653cc633c90f14c&revision=2cace8b4-9ed2-42a7-b653-cc633c90f14c&hash=72926ABEB0033893493CDE9D0801832F

if [ ! -f "${ARCHIVE_NAME}" ]; then
   wget "${BASE_URL}/${VERSION}/binrel/${ARCHIVE_NAME}"
fi

cd "${STOW_DIR}"
${sudo_package} tar -xf "${BUILD_DIR}/${ARCHIVE_NAME}"
${sudo_package} mkdir "${PACKAGE}/share/doc/${PACKAGE}"
${sudo_package} mv "${PACKAGE}"/*.txt "${PACKAGE}/share/doc/${PACKAGE}"
${sudo_package} chown -R $(stat -c "%u:%g" "${STOW_DIR}") "${STOW_DIR}/${PACKAGE}"

stow_package
