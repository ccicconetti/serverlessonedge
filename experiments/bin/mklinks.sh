#!/bin/bash

if [ $# -ne 1 ] ; then
  echo "syntax: $0 <debug|release>"
  exit -1
fi

exec_root=../../build/$1
exec_directories="$exec_root/Executables $exec_root/etsimec/Executables"

for exec_directory in $exec_directories ; do
  if [[ ! -d $exec_directory ]] ; then
    echo "Build $1 not present"
    exit -1
  fi
done

for exec_directory in $exec_directories ; do
  for i in $(cat .gitignore) ; do
    ln -fs $(find $exec_directory -name $i) . 2> /dev/null
  done
done
