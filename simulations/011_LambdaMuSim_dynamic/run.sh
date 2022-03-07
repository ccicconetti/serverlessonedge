#!/bin/bash

if [ "$DRY" == "" ] ; then
  mkdir data 2> /dev/null
fi

epochs="3.6e6"

for e in $epochs ; do
  cmd="./lambdamusim \
        --type dynamic \
        --nodes-path network/urban_sensing.0.nodes \
        --links-path network/urban_sensing.0.links \
        --edges-path network/urban_sensing.0.edges \
        --cloud-distance-factor 2 \
        --duration 8.64e7 \
        --warm-up $e \
        --epoch $e \
        --min-periods 50 \
        --avg-apps 100 \
        --alpha 0.5 \
        --beta 0.5 \
        --lambda-request 1 \
        --out-file data/urban0.csv \
        --append \
        --seed-starting 1 \
        --num-replications 64"

  if [ "$DRY" == "" ] ; then
    $cmd
  else
    echo $cmd
  fi
done
