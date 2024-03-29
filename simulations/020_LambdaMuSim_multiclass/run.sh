#!/bin/bash

if [ "$SEED_STARTING" == "" ] ; then
  SEED_STARTING=1
fi

if [ "$NUM_REPLICATIONS" == "" ] ; then
  NUM_REPLICATIONS=6000
fi

if [ "$NUM_THREADS" == "" ] ; then
  NUM_THREADS=1
fi

if [[ "$DRY" == "" && ! -d "data" ]] ; then
  mkdir data 2> /dev/null
fi

numapps="20 40 60 80 100 120 140 160 180 200"
algos="random greedy proposed"

for n in $numapps ; do
for a in $algos ; do

  mu_algo=$a
  lambda_algo=$a
  if [ "$a" == "proposed" ] ; then
    mu_algo="hungarian"
    lambda_algo="mcfp"
  fi

  cmd="./lambdamusim \
        --type snapshot \
        --nodes-path network/urban_sensing.0.nodes \
        --links-path network/urban_sensing.0.links \
        --edges-path network/urban_sensing.0.edges \
        --cloud-distance-factor 2 \
        --cloud-storage-cost-local 0 \
        --cloud-storage-cost-remote 10 \
        --avg-lambda $(( n * 3 / 4 )) \
        --avg-mu $(( n / 4 )) \
        --alpha 0.5 \
        --beta 1 \
        --app-model classes,0.5,1,100,10,0.5,10,1,0 \
        --mu-algorithm $mu_algo \
        --lambda-algorithm $lambda_algo \
        --out-file data/020-urban0.csv \
        --append \
        --seed-starting $SEED_STARTING \
        --num-replications $NUM_REPLICATIONS \
        --num-threads $NUM_THREADS \
        "

  if [ "$EXPLAIN" != "" ] ; then
    $cmd --explain
    exit 0
  elif [ "$PRINT" != "" ] ; then
    $cmd --print
    exit 0
  elif [ "$DRY" == "" ] ; then
    $cmd
  else
    echo $cmd
  fi

done
done

