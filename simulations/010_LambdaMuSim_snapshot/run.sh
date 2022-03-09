#!/bin/bash

if [ "$DRY" == "" ] ; then
  mkdir data 2> /dev/null
fi

mus="25 50 75"
alphas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"

for m in $mus ; do
  for a in $alphas ; do

    cmd="./lambdamusim \
          --type snapshot \
          --nodes-path network/urban_sensing.0.nodes \
          --links-path network/urban_sensing.0.links \
          --edges-path network/urban_sensing.0.edges \
          --cloud-distance-factor 2 \
          --avg-lambda 50 \
          --avg-mu $m \
          --alpha $a \
          --beta 0.5 \
          --lambda-request 1 \
          --out-file data/010-urban0.csv \
          --append \
          --seed-starting 1 \
          --num-replications 6400"

    if [ "$DRY" == "" ] ; then
      $cmd
    else
      echo $cmd
    fi
  done
done

