#!/bin/bash

# exit immediately if a command exits with a non-zero status
set -e

SRC_FILES="main.c"
OUTPUT_EXE="main"
CC="gcc"
FLAGS="-Wall -Wextra -std=c11 -lm -pthread"
PERF_FLAGS="-O3 -march=native"

build() {
    $CC $SRC_FILES $FLAGS $PERF_FLAGS -o $OUTPUT_EXE
    chmod +x $OUTPUT_EXE
}

run() {
    build
    ./$OUTPUT_EXE $@ 
}

if [ "$1" = "build" ]; then
    build
elif [ "$1" = "run" ]; then
    shift
    run $@
else
    echo "usage: $0 {build|run}"
    exit 1
fi
