#!/bin/bash

cd "$(dirname $0)"
grep -R \
  --exclude-dir build \
  --exclude-dir deps \
  --exclude-dir .git \
  --exclude-dir '*.dSYM' \
  --exclude '*.swp' \
  -I \
  -n \
  "$@" \
  .
