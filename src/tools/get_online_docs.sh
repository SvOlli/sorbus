#!/bin/sh

set -e
cd "$(dirname "${0}")/../../doc"
mkdir -p downloaded
cd downloaded
grep -i -e '\.pdf' -e '\.md' ../site/datasheets.md | sed -e 's/.*(\(.*\)).*/\1/' |
  xargs wget -c \
    --user-agent="Mozilla/5.0 (X11; Linux x86_64; rv:91.0) Gecko/20100101 Firefox/91.0" \

