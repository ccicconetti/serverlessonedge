#!/bin/bash

python2 create_topo.py \
  --in nodes.coord \
  --lines nodes.lines \
  --topo nodes.topo \
  --distance 0.045 \
  --collapse 0.012 \
  --out nodes.out
