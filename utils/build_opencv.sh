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
echo "INSTALLING SYSTEM PACKAGES"

apt update
apt install -y \
  libgtk2.0-dev libavcodec-dev libavformat-dev libswscale-dev

echo "**********************************************************************"
echo "INSTALLING OPENCV"

if [ -r /usr/local/lib/libopencv_core.so.4.1.1 ] ; then
  echo "it seems it is already installed, skipping"
else
  git clone https://github.com/opencv/opencv.git
  git clone https://github.com/opencv/opencv_contrib.git
  pushd opencv_contrib
  git checkout 4.1.1
  popd
  pushd opencv
  git checkout 4.1.1
  mkdir build
  pushd build
  cmake -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    ..
  make -j$CONCURRENCY install
  popd
  popd

  if [ ! -r /usr/local/lib/libopencv_core.so.4.1.1 ] ; then
    echo "installation failed"
    exit 1
  fi
fi

ldconfig

