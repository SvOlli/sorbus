#!/bin/bash

set -e

cd "$(dirname "${0}")/../.."

readonly basedir="${PWD}"

opcode_cvs2md()
{
   local cpu="${1}"
   local cpu65816=""
   if [ "${cpu}" == "65816" ]; then
      cpu65816=" (8 bit mode for 65816)"
   fi

   cat <<EOH
# ${cpu^^} Opcodes

- Name: name of instruction (e.g. "LDA")
- Mode: addressing mode (e.g. "REL" for branch)
- Reserved: is it a resevered / undocumented (aka illegal) opcode
- Bytes: bytes used for command${cpu65816}
- Cycles: cycles taken (without extra for e.g. page crossing)
- ExtraCycles: extra cycles taken when crossing a page(a) and/or taking a branch(b)

EOH

   local header=1
   while read opcode name mode reserved bytes cycles extracycles jump mxe; do
      case "${reserved}" in
      1) reserved=x;;
      esac
      case "${extracycles}" in
      1) extracycles=a;;
      2) extracycles=b;;
      esac
      echo "| ${opcode} | ${name} | ${mode} | ${reserved} | ${bytes} | ${cycles} | ${extracycles} |"
      if [ ${header} -eq 1 ]; then
         echo "| :---- | :---- | :---- | ----: | ----: | ----: | ----: |"
         header=0
      fi
   done

if [ "${cpu}" == "65ce02" ]; then
cat <<EOF

Also note that the 65CE02 uses the term basepage (BP) instead of zeropage (ZP),
since it is moveable. However, for better comparision with other CPUs, the term
zeropage (ZP) was kept here. Also, this processor does not require any extra
cycles for branches or indexed addressing modes.
EOF
fi
}

for cpu in 6502 65sc02 65c02 65ce02 65816; do
   IFS=';' opcode_cvs2md "${cpu}" <doc/opcodes${cpu}.csv >doc/site/opcodes/opcodes_${cpu}.md
done

cd doc
case "${1}" in
upload) mkdocs build --clean &&
        rsync -avz --modify-window=5 --delete-before \
           ../build/site/ \
           sorbus.xayax.net:/srv/www/root/sorbus.xayax.net/
;;
*) exec mkdocs "${@}";;
esac

