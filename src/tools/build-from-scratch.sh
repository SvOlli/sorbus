#!/bin/sh

set -e
cd "$(dirname "${0}")/../.."

readonly logfile="$(basename "${0}" .sh).log"
readonly releasefile="SorbusComputerCores.zip"


mybanner()
{
   echo
   echo "**${*}**" | sed 's/./*/g'
   echo "* ${*} *"
   echo "**${*}**" | sed 's/./*/g'
   date
   echo
}


build()
{
   mybanner "Setup"
   make setup-apt setup-external

   # Version 1.58 is broken
   # http://deb.debian.org/debian/pool/main/6/64tass/64tass_1.59.3120-1_arm64.deb
   mybanner "Build llvm-mos-sdk"
   src/tools/external-llvm-mos-sdk.sh
   mybanner "Build cpm65"
   src/tools/external-cpm65.sh
   mybanner "Build taliforth2"
   src/tools/external-taliforth2.sh
   mybanner "Build release"
   make release
   mybanner "Done"
}


build 2>&1 | tee "${logfile}"
7z a -mx=9 -bd -sdel "${releasefile}" "${logfile}"
