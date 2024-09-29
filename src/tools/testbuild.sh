#!/bin/sh

set -ex

readonly testdir="../testbuild"
readonly archive="SorbusComputerCores.zip"

cd "$(dirname "${0}")/../.."

rm -rf "${testdir}"
git clone . "${testdir}"
cd "${testdir}"

src/tools/gen_opcode_tables.sh
git status | grep -q 'nothing to commit, working tree clean' ||
{
   git status
   exit 1
}

make release
7z l "${archive}"
ls -l "${archive}"
