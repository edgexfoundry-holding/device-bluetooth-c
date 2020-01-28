#!/bin/sh
set -e -x

# Dependencies
if [ ! -d deps ]; then
  mkdir deps

  cd deps

  # install libcbor used for building the SDK
  git clone https://github.com/PJK/libcbor
  cd libcbor
  git checkout v0.5.0
  sed -e 's/-flto//' -i CMakeLists.txt
  cmake -DCMAKE_BUILD_TYPE=Release -DCBOR_CUSTOM_ALLOC=ON -DCMAKE_INSTALL_LIBDIR=lib .
  make
  make install

  # get c-sdk from edgexfoundry
  git clone https://github.com/edgexfoundry/device-sdk-c.git
  cd device-sdk-c
  git checkout v1.0.2

  ./scripts/build.sh
  cp -rf include/* /usr/include/
  cp build/debug/c/libcsdk.so /usr/lib/
  mkdir -p /usr/share/doc/edgex-csdk
  cp Attribution.txt /usr/share/doc/edgex-csdk
  rm -rf /device-ble-c/deps
fi
