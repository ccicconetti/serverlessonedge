#!/bin/bash

apps=100
mixes="lambdas mus mix"
columns=(15 16 17)
names=("lambda-cost" "mu-cost" "mu-cloud")

mkdir pm3d 2> /dev/null

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

  echo $m

  for i in ${!columns[@]}; do
    ./pm3d.py  --lambda $avg_lambda --mu $avg_mu --col ${columns[$i]} \
      < ../data/012-urban0.csv > pm3d/${names[$i]}-$m.dat 
  done
done
