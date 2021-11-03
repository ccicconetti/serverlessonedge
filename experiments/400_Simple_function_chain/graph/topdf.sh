#!/bin/bash

pdfplot.sh -P 8,6 -s 30 -c chain-delay-avg-*-ci.plt
pdfplot.sh -c -s 30 -P 18,6 chain-tpt-histo-ci.plt
