#!/bin/bash

if [ ! -d proxygen ] ; then
  echo "**********************************************************************"
  echo "BUILDING PROXYGEN"
  git clone https://github.com/facebook/proxygen.git
  pushd proxygen
  git checkout v2020.12.07.00 #v2020.10.12.00
  pushd proxygen
  patch -p0 < ../../patches/proxygen
  ./build.sh -q >& build.log
  RET=$?
  popd
  popd

  if [ $RET -ne 0 ] ; then
    echo "build failed, see proxygen/proxygen/build.log for details"
    exit 1
  fi
fi

echo "**********************************************************************"
echo "CREATING SYMLINKS"
rm -rf include libs 2> /dev/null
mkdir include libs 2> /dev/null
pushd libs
ln -s $(find ../proxygen/proxygen/_build_shared/ -name "*.so*" | grep -v '/deps/lib/') .
popd

pushd proxygen
tar cf ../tmp1.$$.tar $(find proxygen -name *.h | grep -v _build)
pushd proxygen/_build_shared/generated
tar cf ../../../../tmp2.$$.tar $(find . -name *.h)
popd
popd
pushd include
ln -s ../proxygen/proxygen/_build_shared/deps/include/* .
tar xf ../tmp1.$$.tar
tar xf ../tmp2.$$.tar
popd
rm -f tmp[12].$$.tar
