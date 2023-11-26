#!/bin/sh

set -ex

readonly testdir="../testbuild"
readonly archive="SorbusComputerCores.zip"

cd "$(dirname "${0}")/../.."

rm -rf "${testdir}"
git clone . "${testdir}"
cd "${testdir}"
make release
7z l "${archive}"
ls -l "${archive}"
