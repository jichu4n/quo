#!/bin/bash

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
m0_dir="$root_dir/m0"
m1_dir="$root_dir/m1"

set -ex

cd "$m1_dir"

flex --outfile=lexer.cpp lexer.l
bison --header=parser.h --output=parser.cpp parser.y

node "$m0_dir/dist/quo-m0.js" \
		-o quo-m1 \
    *.quo \
		lexer.cpp \
		parser.cpp \
