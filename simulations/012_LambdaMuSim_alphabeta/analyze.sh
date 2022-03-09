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

apps=100
mixes="lambdas mus mix"
alphas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"
betas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"

infile=data/012-urban0.csv

columns=(11 12 15 16 17)
names=("num-containers" "tot-capacity" "lambda-cost" "mu-cost" "mu-cloud")

for m in $mixes ; do
  avg_lambda=0
  avg_mu=0
  if [ $m == "lambdas" ] ; then
    avg_lambda=$apps
  elif [ $m == "mus" ] ; then
    avg_mu=$apps
  elif [ $m == "mix" ] ; then
    avg_lambda=$(( apps / 2 ))
    avg_mu=$(( apps / 2 ))
  else
    echo "unknown mix: $m, bailing out"
    exit 1
  fi

  for a in $alphas ; do
    for i in ${!columns[@]} ; do
      outmangle=${names[$i]}-$m-alpha=$a
      echo "$outmangle"
      outfile=post/$outmangle.dat
      rm -f $outfile 2> /dev/null
      for b in $betas ; do
        fingerprint=",2.000000,60000.000000,50,10,$avg_lambda,$avg_mu,$a""00000,$b""00000,1,"
        value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
        echo $b $value >> $outfile
      done
    done
  done

  for b in $betas ; do
    for i in ${!columns[@]} ; do
      outmangle=${names[$i]}-$m-beta=$b
      echo "$outmangle"
      outfile=post/$outmangle.dat
      rm -f $outfile 2> /dev/null
      for a in $alphas ; do
        fingerprint=",2.000000,60000.000000,50,10,$avg_lambda,$avg_mu,$a""00000,$b""00000,1,"
        value=$(grep $fingerprint $infile | $percentile_script --delimiter , --column ${columns[$i]} --mean | cut -f 1,3 -d ' ')
        echo $a $value >> $outfile
      done
    done
  done
done
