#!/bin/bash

size=$(../bin/forwardingtableclient --server $1 | grep -v '^Table' | wc -l)
echo $1 $((size/2))
