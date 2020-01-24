#!/bin/sh
set -e -x

# Find root directory and system type
ROOT=$(dirname $(dirname $(readlink -f $0)))

cd $ROOT

mkdir -p $ROOT/build/release/device-ble-c
cd $ROOT/build/release/device-ble-c

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release $ROOT
make 2>&1 | tee release.log