#!/bin/bash

set -e

doshift=1
for i; do
case "$i" in
-p=*)
   BUILD_DIR="${i#-p=}"
;;
*clang-tidy)
   doshift=0
;;
*)
   if [ ${doshift} -ne 0 ]; then
      eval "$i"
      shift
   fi
;;
esac
done

getincludes()
{
local p=0
${CC} -xc -E -v - </dev/null 2>&1 >/dev/null | while read line; do case "${line}" in
"#include <...> search starts here:") p=1;;
"End of search list.") p=0;;
/*) [ ${p} -ne 0 ] && echo -n "-I${line} ";;
esac;done
}

[ -n "${BUILD_DIR}" ]

sed \
   -e "s,-mcpu=cortex-m0plus,-I/usr/include/newlib,g" \
   -e "s,-mthumb,$(getincludes),g" \
   -e "s/-isystem /-I/g" \
   -i "${BUILD_DIR}/compile_commands.json"

echo $@

exec $@
