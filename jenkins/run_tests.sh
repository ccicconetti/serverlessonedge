#!/bin/bash

if [ "$WORKSPACE" == "" ] ; then
  WORKSPACE=$PWD
fi

if [ "$COMPILER" == "" ] ; then
  COMPILER=g++
fi

if [ "$CONCURRENCY" == "" ] ; then
  CONCURRENCY=5
fi

MODULES=". etsimec etsimec/rest etsimec/rest/support"

echo "WORKSPACE:   ${WORKSPACE}"
echo "COMPILER:    ${COMPILER}"
echo "CONCURRENCY: ${CONCURRENCY}"
echo "MODULES:     ${MODULES}"

ret=0
counter=0
for module in $MODULES ; do

  echo "################################################################################"
  echo "module: $module"

  cd ${WORKSPACE}/$module/build/debug

  echo "cleaning previous compilation"
  rm -rf * >& /dev/null

  ../build.sh ${COMPILER}

  if [ $? -ne 0 ]; then
    # exit immediately if cmake fails
    echo "$module: cmake failed"
    exit 1
  fi

  make -j${CONCURRENCY}

  if [ $? -ne 0 ] ; then
    # exit immediately if make fails
    echo "$module: make failed"
    exit 1
  fi

  counter=$(( counter + 1 ))
  Test/testmain \
    --gtest_shuffle \
    --gtest_output=xml:${WORKSPACE}/testunit-$counter.xml

  if [ $? -ne 0 ] ; then
    # return error even if a single test fails, but continue running them
    ret=1
  fi
done

exit $ret
