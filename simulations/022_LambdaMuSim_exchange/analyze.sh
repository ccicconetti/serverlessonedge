#!/bin/bash

if [ ! -d "data" ] ; then
  echo "no 'data' directory, bailing out..."
  exit 1
fi

if [ ! -d "post" ] ; then
  mkdir post 2> /dev/null
fi

. ../../scripts/download-percentile.sh

infile=data/022-urban0.csv

columns=(21 22 23)
names=("mu-cost" "mu-cloud" "mu-service-cloud")

numapps="25 50"
algos="random greedy proposed"
exchange_sizes="0 2 4 6 8 10 12 14 16 18 20"

for n in $numapps ; do
for a in $algos ; do

  algo_fp=$a,$a
  if [ "$a" == "proposed" ] ; then
    algo_fp=hungarian,mcfp
  fi

  for i in ${!columns[@]} ; do
    outmangle=${names[$i]}-$n-$a
    echo "$outmangle"
    outfile=post/$outmangle.dat
    rm -f $outfile 2> /dev/null
    for e in $exchange_sizes ; do
      fingerprint="2.000000,0.000000,10.000000,60000.000000,50,10,0,$n,0.500000,1.000000,classes;0.5;5;100;10;0.5;5;$e;0,$algo_fp"
      value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
      echo $e $value >> $outfile
    done
  done
 
done
done
