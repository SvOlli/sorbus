#!/bin/sh

cd "$(dirname "${0}")"
grep '\.pdf' datasheets.md | sed -e 's/.*\[\(.*\)\].*/\1/' | xargs wget -c

