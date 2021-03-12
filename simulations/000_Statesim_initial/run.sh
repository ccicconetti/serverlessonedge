#!/bin/bash

ops_values="10000 100000 1000000"
jobs_values="50 150 300 500"
net_values="urban_sensing iiot"

mem=1000000

if [ -z "${NTHREADS}" ] ; then
  NTHREADS=50
fi

./create_tasks.sh

mkdir logs 2> /dev/null
for ops in $ops_values ; do
  for jobs in $jobs_values ; do
    for net in $net_values ; do
      echo -n "."
      mangle=net=$net.ops=$ops.jobs=$jobs
      ./statesim \
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
        --state-factor $mem \
        --num-jobs $jobs \
        --outdir data/$mangle \
        >& logs/$mangle.log
    done
  done
done
echo
