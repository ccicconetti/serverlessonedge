#!/bin/bash

if [ ! -d "data" ] ; then
  echo "no 'data' directory"
fi

percentile_script=$(which percentile.py)
if [ "$percentile_script" == "" ] ; then
  if [ ! -x percentile.py ] ; then
    curl -opercentile.py https://raw.githubusercontent.com/ccicconetti/serverlessonedge/master/scripts/percentile.py >& /dev/null
    if [ $? -ne 0 ] ; then
      echo "error downloading the percentile.py script"
    fi
    chmod 755 percentile.py
  fi
  percentile_script=./percentile.py
fi

if [ ! -d "post" ] ; then
  mkdir post 2> /dev/null
fi

apps="50 100 200"
epochs="3.6e4 3.6e5 3.6e6"

infile=data/urban0.csv

columns=(11 12 15 16 17)
names=("num-containers" "tot-capacity" "lambda-cost" "mu-cost" "mu-cloud")

for a in $apps ; do
  for i in ${!columns[@]}; do
    outmangle=${names[$i]}-$a
    echo "$outmangle"
    outfile=post/$outmangle.dat
    rm -f $outfile 2> /dev/null
    for e in $epochs ; do
      fingerprint="2.000000,$e,50,$a,10,10,0.500000,0.500000,1"
      value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
      echo $a $value >> $outfile
    done
  done
done
