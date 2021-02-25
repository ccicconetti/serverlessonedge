#!/bin/bash

if [ -z "${SIZE}" ] ; then
  SIZE=512m
fi

if [ -z "${DIR}" ] ; then
  DIR=data
fi

if [ ! -d $DIR ] ; then
  mkdir $DIR 2> /dev/null
  if [ $? == 1 ] ; then
    echo "Error creating directory: $DIR"
    exit 1
  fi
fi

read -n 1 -p "Mounting tmpfs of size $SIZE on directory $DIR, proceed? (Ctrl+C to abort)"

sudo mount -t tmpfs -o size=$SIZE tmpfs $DIR
sudo chown $USER $DIR
