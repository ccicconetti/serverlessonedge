#!/bin/bash

mkdir data 2> /dev/null

if [ -r data/012-urban0.csv ] ; then
  echo "raw data file already present: cowardly refusing to proceed"
  exit 1
else
  wget http://turig.iit.cnr.it/~claudio/public/012-urban0.csv.gz -O- | gzip -dc > data/012-urban0.csv
fi
