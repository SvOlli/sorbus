#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

for trace in doc/traces/*.trace; do
   disass="${trace%.trace}.disass"
   reference="${trace%.trace}.ref"
   diff="${trace%.trace}.diff"
   cpu="${trace##*/}"
   cpu="${cpu%%.*}"

   build/tools/historian.exe -c "${cpu}" -f "${trace}" -t \
      > "${disass}"
   if [ -f "${reference}" ]; then
      diff -bu "${reference}" "${disass}" > "${diff}" || :
   fi
done

for i in doc/traces/*.diff; do
   if [ -f "${i}" -a ! -s "${i}" ]; then
      rm -f "${i}"
   fi
done

ls -l doc/traces/*.diff
