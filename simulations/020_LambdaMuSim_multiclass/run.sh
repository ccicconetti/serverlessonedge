#!/bin/bash

if [ "$DRY" == "" ] ; then
  mkdir data 2> /dev/null
fi

apps=100
mixes="lambdas mus mix"
alphas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"
betas="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"

for m in $mixes ; do
  for a in $alphas ; do
    for b in $betas ; do

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

      cmd="./lambdamusim \
            --type snapshot \
            --nodes-path network/urban_sensing.0.nodes \
            --links-path network/urban_sensing.0.links \
            --edges-path network/urban_sensing.0.edges \
            --cloud-distance-factor 2 \
            --avg-lambda $avg_lambda \
            --avg-mu $avg_mu \
            --alpha $a \
            --beta $b \
            --lambda-request 1 \
            --out-file data/012-urban0.csv \
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
done

