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

echo "WORKSPACE:   ${WORKSPACE}"
echo "COMPILER:    ${COMPILER}"
echo "CONCURRENCY: ${CONCURRENCY}"


BUILD_TYPES="debug release"

BUILD_DIRS="
  build
  etsimec/build
  etsimec/rest/build
  etsimec/rest/support/build
  "

FAILED_BUILD=""
FAILED_MAKE=""
for t in $BUILD_TYPES ; do
  for d in $BUILD_DIRS ; do
    pushd $WORKSPACE/$d/$t
    if [ $? -ne 0 ] ; then
      FAILED_BUILD="$FAILED_BUILD $d/$t"
      continue
    fi
    if [ "$(ls -A)" != "" ] ; then
      rm -rf *
    fi
    ../build.sh $COMPILER
    if [ $? -ne 0 ] ; then
      FAILED_BUILD="$FAILED_BUILD $d/$t"
      popd
      continue
    fi

    make -j$CONCURRENCY
    if [ $? -ne 0 ] ; then
      FAILED_MAKE="$FAILED_MAKE $d/$t"
    fi
    popd
  done
done

if [[ $FAILED_BUILD == "" && $FAILED_MAKE == "" ]] ; then
  exit 0
fi

echo "the following build commands have failed: $FAILED_BUILD"
echo "the following make commands have failed:  $FAILED_MAKE"

exit 1
