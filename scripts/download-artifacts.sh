#!/bin/bash

if [ -d results ] ; then
  echo "results directory already present: cowardly refusing to proceed"
  exit 1
fi

ID=$(basename $PWD | cut -f 1 -d '_')
wget http://turig.iit.cnr.it/~claudio/public/quantum-routing/soe-results-$ID.tgz -O- | tar zx
