#!/bin/bash

if [ ! -d proxygen ] ; then
  echo "**********************************************************************"
  echo "BUILDING PROXYGEN"
  git clone https://github.com/facebook/proxygen.git
  pushd proxygen
  git checkout v2020.10.12.00
  pushd proxygen
  ./build.sh -q >& build.log
  RET=$?
  popd
  popd

  if [ $RET -ne 0 ] ; then
    echo "build failed, see proxygen/proxygen/build.log for details"
    exit 1
  fi
fi

if [ ! -d oktoplus ] ; then
  echo "**********************************************************************"
  echo "BUILDING OKTOPLUS"
  git clone https://github.com/ccicconetti/oktoplus.git
  pushd oktoplus
  git submodule update --init --recursive
  mkdir build/optimized
  pushd build/optimized
  ../buildme.sh >& build.log
  RET=$?
  popd
  popd

  if [ $RET -ne 0 ] ; then
    echo "build failed, see oktoplus/build/optimized/build.log for details"
    exit 1
  fi
fi
