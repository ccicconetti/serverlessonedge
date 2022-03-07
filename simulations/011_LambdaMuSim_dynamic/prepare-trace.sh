#!/bin/bash

if [ ! -x afdb-convert ] ; then
  echo "Cannot find executable: afdb-convert"
  exit 1
fi

if [ ! -r azurefunctions-accesses-2020-timestamps ] ; then
  if [ ! -r azurefunctions-accesses-2020-sorted.csv ] ; then
    echo "Downloading Azure trace of stateful FaaS applications, to be decompressed and sorted chronologically"
    wget https://azurecloudpublicdataset2.blob.core.windows.net/azurepublicdatasetv2/azurefunctions_dataset2020/azurefunctions-accesses-2020.csv.bz2
    bzip2 -dc azurefunctions-accesses-2020.csv.bz2 | sort -g > azurefunctions-accesses-2020-sorted.csv
  fi

  ./afdb-convert --input-raw azurefunctions-accesses-2020-sorted.csv --output-timestamp azurefunctions-accesses-2020-timestamps 2> /dev/null
fi
