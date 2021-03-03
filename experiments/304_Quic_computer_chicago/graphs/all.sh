#!/bin/bash

pdfcairo="uncoord-chicago-out-090.plt uncoord-chicago-out-095.plt uncoord-chicago-out-mean.plt uncoord-chicago-tpt-mean.plt uncoord-chicago-util-mean.plt"
postscript="uncoord-chicago-topo.plt"

for i in $pdfcairo ; do
  pdfplot.sh -s 26 $i
done

for i in $postscript ; do
  pdfplot.sh -s 26 -p $i
done
