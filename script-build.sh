#!/bin/bash
#simple fast build script

if [[ -d "./build" ]]; then
    rm -r build
fi

mkdir build && cd build
cmake .. && cmake --build . -j $(nproc)