#!/bin/bash

if [ -z $CONCURRENCY ] ; then
  CONCURRENCY=4
fi

distro=$(lsb_release -sc 2> /dev/null)

if [ "$distro" != "bionic" ] ; then
  echo "Error: script only tested on Ubuntu 18.04 (Bionic)"
  exit 1
fi

echo "**********************************************************************"
echo "INSTALLING OR-TOOLS"

if [ -r /usr/local/lib/libortools.so.9.2.9974 ] ; then
  echo "it seems it is already installed, skipping"
else
  git clone https://github.com/google/or-tools.git
  pushd or-tools
  git checkout stable
  mkdir build
  pushd build
  cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_CXX:BOOL=ON \
    -DBUILD_Cbc:BOOL=ON \
    -DBUILD_Cgl:BOOL=ON \
    -DBUILD_Clp:BOOL=ON \
    -DBUILD_CoinUtils:BOOL=ON \
    -DBUILD_Osi:BOOL=ON \
    -DBUILD_SCIP:BOOL=ON \
    -DBUILD_absl:BOOL=ON \
    ..
  make -j$CONCURRENCY install
  popd
  popd

  if [ ! -r /usr/local/lib/libortools.so.9.2.9974 ] ; then
    echo "installation failed"
    exit 1
  fi
fi

ldconfig

