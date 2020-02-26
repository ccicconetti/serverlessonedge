#!/bin/bash

if [ $# -ne 1 ] ; then
  echo "Syntax: `basename $0` <compiler_path>"
  exit 1
fi

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

cmake -DCMAKE_CXX_COMPILER=$1 -DCMAKE_BUILD_TYPE=${CURRENTDIR} ../../
