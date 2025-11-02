#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

readonly basedir="${PWD}"
readonly space=" "
readonly tab="$(echo | tr '\n' '\t')"
readonly cr="$(echo | tr '\n' '\r')"

stripof()
{
   local regexp="${1}${1}"'*$'
   find src/65c02 src/rp2040 src/tools -type f -name '*.cc' -o -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.s' -o -name '*.inc' |
      xargs grep -l "${regexp}" |
      tee /dev/stderr |
      xargs sed -i -e "s/${regexp}//g" || :
}

echo >&2 "removing trailing carrage returns"
stripof "${cr}"

echo >&2 "removing trailing spaces"
stripof "${space}"

echo >&2 "removing trailing tabs"
stripof "${tab}"
