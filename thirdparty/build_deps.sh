#!/bin/bash

if [ ! -d proxygen ] ; then
  echo "**********************************************************************"
  echo "BUILDING PROXYGEN"
  git clone https://github.com/facebook/proxygen.git
  pushd proxygen
  git checkout v2020.12.07.00 #v2020.10.12.00
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
