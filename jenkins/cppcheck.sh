#!/bin/bash

CPPCHECK=/usr/local/bin/cppcheck
OPTIONS="--library=gnu --library=posix \
         --platform=unix64 --std=c++17 \
         --suppressions-list=cppcheck.suppression \
         --enable=all \
         --xml \
         --xml-version=2 \
         -I../emulator \
         -I../build/debug/emulator/proto-src \
         --suppress=syntaxError \
         -j 8 \
         --force \
         --output-file=report.xml"
         
#-I../build/debug/googletest-src/googletest/include \

${CPPCHECK} ${OPTIONS} \
  ../Edge \
  ../Executables \
  ../Factory \
  ../OpenCV \
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
