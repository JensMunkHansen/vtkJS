#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 [wasm|static|shared]"
    exit 1
fi

# Convert parameter to lowercase for case-insensitive comparison
param=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [ "$param" = "static" ]; then
    rm -Rf build
    cmake -S. -Bbuild -DBUILD_SHARED_LIBS=OFF
    cmake --build build --target main
elif [ "$param" = "wasm" ]; then
(
    rm -Rf build
    source "$HOME/github/emsdk/emsdk_env.sh"
    emcmake cmake -S. -Bbuild
    cmake --build build --target main
)
else
    echo "Invalid argument. Usage: $0 [wasm|static|shared]"
    exit 1    
fi
