#!/bin/bash

if [ "$DRY" == "" ] ; then
  mkdir data 2> /dev/null
fi

apps="50 100 200"
epochs="60e3 300e3 600e3 1200e3 1800e3 3600e3"

for e in $epochs ; do
  for a in $apps ; do
    cmd="./lambdamusim \
          --type dynamic \
          --nodes-path network/urban_sensing.0.nodes \
          --links-path network/urban_sensing.0.links \
          --edges-path network/urban_sensing.0.edges \
          --cloud-distance-factor 2 \
          --apps-path azurefunctions-accesses-2020-timestamps \
          --duration 8.64e7 \
          --warm-up $e \
          --epoch $e \
          --min-periods 50 \
          --avg-apps $a \
          --alpha 0.5 \
          --beta 0.5 \
          --lambda-request 1 \
          --out-file data/urban0.csv \
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
