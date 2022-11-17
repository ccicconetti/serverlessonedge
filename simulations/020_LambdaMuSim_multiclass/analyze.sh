#!/bin/bash

if [ ! -d "data" ] ; then
  echo "no 'data' directory, bailing out..."
  exit 1
fi

if [ ! -d "post" ] ; then
  mkdir post 2> /dev/null
fi

. ../../scripts/download-percentile.sh

infile=data/020-urban0.csv

columns=(15 16 19 20 21 22 23)
names=("num-containers" "tot-capacity" "lambda-cost" "lambda-service-cloud" "mu-cost" "mu-cloud" "mu-service-cloud")

numapps="10 20 50 100"
algos="random greedy proposed"

for a in $algos ; do

  algo_fp=$a,$a
  if [ "$a" == "proposed" ] ; then
    algo_fp=hungarian,mcfp
  fi

  for i in ${!columns[@]} ; do
    outmangle=${names[$i]}-$a
    echo "$outmangle"
    outfile=post/$outmangle.dat
    rm -f $outfile 2> /dev/null
    for n in $numapps ; do
      fingerprint="2.000000,0.000000,100.000000,60000.000000,50,10,$((n/2)),$((n/2)),0.500000,1.000000,classes;0.5;1;100;1;0.5;10;1;10,$algo_fp"
      value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
      echo $n $value >> $outfile
    done
  done

done
