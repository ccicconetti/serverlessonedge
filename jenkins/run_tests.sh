#!/bin/bash

cd ${WORKSPACE}/build/debug

echo "cleaning previous compilation"
rm -rf * >& /dev/null

../build.sh g++

if [ $? -ne 0 ]; then
  echo "cmake failed"
  exit 1
fi

make -j5

if [ $? -ne 0 ] ; then
  echo "make failed"
fi

Test/testmain \
  --gtest_shuffle \
  --gtest_output=xml:${WORKSPACE}/testunit.xml

exit $?
