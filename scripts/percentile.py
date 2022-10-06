#!/usr/bin/env python2
"""
Compute summary statistics from a dataset on a file
"""

import math
import sys
import argparse
import numpy as np

parser = argparse.ArgumentParser(
    "Simple statistics computer from data in column text files",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--delimiter", type=str, default=None,
                    help="Use the specified delimiter")
parser.add_argument("--warmup", type=float, default=0,
                    help="Skip lines with the warm-up column small than this")
parser.add_argument("--duration", type=float, default=-1,
                    help="Skip lines with the warm-up column bigger than this")
parser.add_argument("--warmup_column", type=int, default=0,
                    help="Which column contains the experiment time, 1-based indexing")
parser.add_argument("--column", type=int, default=1,
                    help="Which column to use, 1-based indexing")
parser.add_argument("--quantiles", nargs="*", type=float,
                    help="Quantile(s) to print")
parser.add_argument("--mean", action="store_true", default=False,
                    help="Print mean value")
parser.add_argument("--count", action="store_true", default=False,
                    help="Print number of values")
parser.add_argument("--histo", action="store_true", default=False,
                    help="Print histograms")
parser.add_argument("--cdf", action="store_true", default=False,
                    help="Print cumulative distribution function")
args = parser.parse_args()

if ((args.quantiles and len(args.quantiles)) > 0 or args.mean or args.count) and \
    (args.histo or args.cdf):
    raise Exception('Cannot mix scalar and vector outputs')

if args.histo and args.cdf:
    raise Exception('Cannot produce both CDF and histogram')

if args.column >= 1:
    index = args.column - 1
else:
    index = args.column
data = []
for line in sys.stdin:
    tokens = line.split(args.delimiter)
    if args.warmup_column > 0:
        time = float(tokens[args.warmup_column - 1])
        if time < args.warmup or (args.duration > 0 and time > args.duration):
            continue

    x = float(tokens[index])
    if not math.isnan(x):
        data.append(x)

if len(data) == 0:
    exit(-1)

if args.mean:
    print '{} +- {}'.format(np.mean(data), np.std(data))

if args.quantiles:
    data.sort()
    for quantile in args.quantiles:
        q = min(int(len(data) * quantile), len(data)-1)
        print '{} {}'.format(quantile, data[q])

if args.count:
    print '{} records'.format(len(data))

if args.histo:
    res = np.histogram(data, bins=10, density=True)
    tot = math.fsum(res[0])
    for x, y in zip(res[1], res[0]):
        print '{} {}'.format(x, y / tot)

if args.cdf:
    for x,y in zip(range(0, len(data)), sorted(data)):
        print '{} {}'.format(y, float(x) / len(data))

