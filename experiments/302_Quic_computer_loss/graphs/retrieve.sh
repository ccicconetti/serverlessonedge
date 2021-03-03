#!/bin/bash

mkdir data 2> /dev/null

for i in $(grep 'data/' *.plt | grep -v topo.plt | cut -f 2 -d "'" | cut -f 2 -d /) ; do
  cp $(find ../derived/ -name $i) data/
done
