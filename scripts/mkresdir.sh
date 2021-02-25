#!/bin/bash

ramdiskpath=$HOME/ramdisk

experiment=$(basename $PWD)

if [ ! -d $ramdiskpath ] ; then
  echo "$ramdiskpath does not exist"
fi

if [ -d $ramdiskpath/$experiment ] ; then
  echo "$ramdiskpath/$experiment already exists"
else
  mkdir $ramdiskpath/$experiment
fi

if [ -d $ramdiskpath/$experiment/results ] ; then
  echo "$ramdiskpath/$experiment/results already exists"
else
  mkdir $ramdiskpath/$experiment/results
fi

if [ -d $ramdiskpath/$experiment/derived ] ; then
  echo "$ramdiskpath/$experiment/derived already exists"
else
  mkdir $ramdiskpath/$experiment/derived
fi

if [ -e results ] ; then
  echo "results already exists: skipped"
else
  ln -s $ramdiskpath/$experiment/results .
fi

if [ -e derived ] ; then
  echo "derived already exists: skipped"
else
  ln -s $ramdiskpath/$experiment/derived .
fi
