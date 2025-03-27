#!/bin/sh

# this tool uses exomizer to compress text used in about application
# exomizer 3.1 or later needs to be installed somewhere in ${PATH}

set -e

readonly blocksize=4096

cd "$(dirname "${0}")/../.."

#rm -f src/65c02/jam/about/*.exo

total_txt=0
total_exo=0
for txt in doc/licenses/*.txt doc/3rdparty/*.txt src/65c02/jam/about/*.txt; do
   txt_size="$(wc -c <"${txt}")"
   pad_size="$(( blocksize - (txt_size % blocksize) ))"
   exo="src/65c02/jam/about/$(basename "${txt}" .txt).exo"
   tmp="${txt%.txt}.tmp"
   if [ ! -f "${exo}" -o "${txt}" -nt "${exo}" ]; then
      # add dummy load address and trailing end marker
      { cat "${txt}" ; dd if=/dev/zero bs=${pad_size} count=1 2>/dev/null ; } >"${tmp}"
      exomizer raw -m ${blocksize} -B -P 0 -o "${exo}" "${tmp}"
      rm -f "${tmp}"
   fi
   total_exo=$((total_exo+$(wc -c <"${exo}")))
   total_txt=$((total_txt+$(wc -c <"${txt}")))
done

echo "total size: ${total_txt} -> ${total_exo} bytes"
touch src/65c02/jam/about/main.s
