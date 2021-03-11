#!/bin/bash

jobs_values="100 200 300 400 500"
net_values="urban_sensing iiot"
apolicy_values="ProcOnly ProcNet"
epolicy_values="UnchainedExternal UnchainedInEdge UnchainedInFunction UnchainedInClient"
metric_values="proclat netlat traffic"
seed_first=1
seed_last=200
quantile_values="0.5 0.9 0.95 0.99"
ops=100000
mem=1000000

mkdir results 2> /dev/null

echo "per chain size results"
for net in $net_values ; do
  for apolicy in $apolicy_values ; do
    for epolicy in $epolicy_values ; do
      for jobs in $jobs_values ; do
        echo -n "."
        mangledir="net=$net.jobs=$jobs"
        manglefile="alloc=$apolicy.exec=$epolicy"
        cat data/$mangledir/job-$manglefile.seed=* | ./per_chain_size.py
        mangleout="net=$net.apolicy=$apolicy.epolicy=$epolicy.jobs=$jobs"
        for metric in $metric_values ; do
          mv $metric.dat results/pcs-$metric.$mangleout
        done
      done
    done
  done
done
echo "done"

echo "increasing number of jobs results"
for net in $net_values ; do
  for apolicy in $apolicy_values ; do
    for epolicy in $epolicy_values ; do
      mangleout="net=$net.apolicy=$apolicy.epolicy=$epolicy"
      rm -f data/tmp*
      for jobs in $jobs_values ; do
        echo -n "."
        mangledir="net=$net.jobs=$jobs"
        manglefile="alloc=$apolicy.exec=$epolicy"
        for (( seed = $seed_first ; seed < $seed_last ; seed++ )) ; do
          filename="data/$mangledir/job-$manglefile.seed=$seed"
          for (( col = 1 ; col <= 3 ; col++ )) ; do
            ret=$(percentile.py --col $col --quantiles $quantile_values --mean < $filename)
            for quantile in $quantile_values ; do
              echo "$jobs $(grep ^"$quantile " <<< $ret | cut -f 2 -d ' ')" >> data/tmp.$col.q=$quantile.$seed
            done
            echo "$jobs $(grep "+-" <<< $ret | cut -f 1 -d ' ')" >> data/tmp.$col.mean.$seed
          done
        done
      done
      for quantile in $quantile_values ; do
        confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.1.q=$quantile.* > results/proclat-$mangleout.q=$quantile.dat
        confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.2.q=$quantile.* > results/netlat-$mangleout.q=$quantile.dat
        confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.3.q=$quantile.* > results/traffic-$mangleout.q=$quantile.dat
      done
      confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.1.mean.* > results/proclat-$mangleout.mean.dat
      confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.2.mean.* > results/netlat-$mangleout.mean.dat
      confidence.py --xcol 1 --ycol 2 --alpha 0.05 data/tmp.3.mean.* > results/traffic-$mangleout.mean.dat
    done
  done
done

rm -f data/tmp*
echo "done"
