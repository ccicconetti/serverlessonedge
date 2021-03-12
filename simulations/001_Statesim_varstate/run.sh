#!/bin/bash

net_values="urban_sensing iiot"
state_values="125000 250000 500000 1000000 2000000 4000000 8000000"

jobs=250
ops=100000
mem=1000000

if [ -z "${NTHREADS}" ] ; then
  NTHREADS=50
fi

./create_tasks.sh

mkdir logs 2> /dev/null

for state in $state_values ; do
  for net in $net_values ; do
    mangle=net=$net.state=$state
    cmd="./statesim \
      --num-functions 10 \
      --num-threads $NTHREADS \
      --seed 1 \
      --num-replications 200 \
      --nodes network/$net.0.nodes \
      --links network/$net.0.links \
      --edges network/$net.0.edges \
      --tasks tasks/batch_task.csv \
      --alloc-policies ProcNet \
      --exec-policies PureFaaS,StatePropagate,StateLocal \
      --ops-factor $ops \
      --arg-factor $mem \
      --state-factor $state \
      --num-jobs $jobs \
      --outdir data/$mangle"

    if [ -z "${DRY}" ] ; then
      echo -n "."
      $cmd >& logs/$mangle.log
    else
      echo "$cmd"
    fi
  done
done
echo "done"
