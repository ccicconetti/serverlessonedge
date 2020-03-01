#!/bin/bash

if [ $# -lt 1 ] ; then
  echo "Syntax: `basename $0` <compiler_path> [CMake options]"
  exit 1
fi

COMPILER=$1

shift 1

if [ ! -z "$( ls -A | grep -v .gitignore)" ]; then
  read -p "Directory not empty, remove everything? (say \"yes\" if sure) "
  if [ "$REPLY" == "yes" ] ; then
    read -n 1 -s -p "This will remove EVERYTHING in the current directory, Ctrl+C to leave NOW"
    rm -rf *
    echo
    echo "Done, the directory should now be empty"
  else
    echo "OK, leaving now"
    exit 1
  fi
fi

CURRENTDIR=${PWD##*/}

cmake \
  -DCMAKE_CXX_COMPILER=$COMPILER \
  -DCMAKE_BUILD_TYPE=${CURRENTDIR} \
  "$@" \
  ../../
