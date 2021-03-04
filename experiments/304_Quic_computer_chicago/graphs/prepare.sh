#!/bin/bash

experiments="centralized distributed"
protocols="grpc quic quic0rtt"

mkdir data 2> /dev/null

for e in $experiments ; do
  for p in $protocols ; do
    rm  data/tpt.$e.$p 2> /dev/null
    for i in ../results/ovsstat..c=40.s=6.e=$e.l=$p.* ; do
      percentile.py --mean --col -1 < $i >> data/tpt.$e.$p
    done
  done
done
