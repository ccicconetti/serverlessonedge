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

mus="25 50 75"
alphas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"

infile=data/urban0.csv

columns=(11 12 15 16 17)
names=("num-containers" "tot-capacity" "lambda-cost" "mu-cost" "mu-cloud")

for m in $mus ; do
  for i in ${!columns[@]}; do
    outmangle=${names[$i]}-$m
    echo "$outmangle"
    outfile=post/$outmangle.dat
    rm -f $outfile 2> /dev/null
    for a in $alphas ; do
      fingerprint=",2.000000,60000.000000,50,10,50,$m,$a""00000,0.500000,1,"
      value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
      echo $a $value >> $outfile
    done
  done
done
