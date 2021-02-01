#!/bin/bash

if [ "$TEXT" == "1" ] ; then
  OPTS="--output-file=report.txt"
else
  OPTS="--xml --xml-version=2 --output-file=report.xml"
fi

CPPCHECK=/usr/local/bin/cppcheck
OPTIONS="--library=gnu --library=posix \
         --platform=unix64 --std=c++17 \
         --suppressions-list=cppcheck.suppression \
         --enable=all \
         --suppress=syntaxError \
         -j 8 \
         --force \
         $OPTS"
         
${CPPCHECK} ${OPTIONS} \
  ../Edge \
  ../Executables \
  ../Factory \
  ../OpenCV \
  ../Quic \
  ../Rpc \
  ../Simulation \
  ../Test \
  ../etsimec/EtsiMec \
  ../etsimec/Executables \
  ../etsimec/OpenWhisk \
  ../etsimec/Test \
  ../etsimec/rest/Rest \
  ../etsimec/rest/Test \
  ../etsimec/rest/support/RpcSupport \
  ../etsimec/rest/support/Support \
  ../etsimec/rest/support/Test
