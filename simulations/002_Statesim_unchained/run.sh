#!/bin/bash

jobs_values="100 200 300 400 500"
net_values="urban_sensing iiot"

ops=100000
mem=1000000

if [ -z "${NTHREADS}" ] ; then
  NTHREADS=50
fi

./create_tasks.sh

mkdir logs 2> /dev/null
for jobs in $jobs_values ; do
  for net in $net_values ; do
    mangle=net=$net.jobs=$jobs
    cmd="./statesim \
      --num-functions 10 \
      --num-threads $NTHREADS \
      --seed 1 \
      --num-replications 200 \
      --nodes network/$net.0.nodes \
      --links network/$net.0.links \
      --edges network/$net.0.edges \
      --tasks tasks/batch_task.csv \
      --alloc-policies ProcOnly,ProcNet \
      --exec-policies UnchainedExternal,UnchainedInEdge,UnchainedInFunction,UnchainedInClient \
      --cloud-rate 50 \
      --cloud-latency 0.06 \
      --ops-factor $ops \
      --arg-factor $mem \
      --state-factor $mem \
      --num-jobs $jobs \
      --outdir data/$mangle"
    if [ -z ${DRY} ] ; then
      echo -n "."
      $cmd >& logs/$mangle.log
    else
      echo $cmd
    fi
  done
done
echo
