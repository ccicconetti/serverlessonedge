#!/bin/bash

mkdir data 2> /dev/null

for i in $(grep 'data/' *.plt | grep -v topo.plt | cut -f 2 -d "'" | cut -f 2 -d /) ; do
  cp $(find ../results/ -name $i) data/
done

(for i in ../results/out.t\=* ; do echo -n "$i: " ; percentile.py --count --warmup_column 1 --warmup 10 < $i; done) > data/count.dat
(for i in ../results/ovsstat.t\=* ; do echo -n "$i: " ; percentile.py --mean --col -1 --warmup_column 1 --warmup 10 < $i ; done) > data/tpt.dat
