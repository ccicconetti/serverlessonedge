#!/usr/bin/python
"""Compute binned statistics"""

import sys
import argparse

parser = argparse.ArgumentParser("Simple binned statistics computer from data in column text files")
parser.add_argument("--warmup", type=float, default=0.0,
                    help="Skip lines with the time column small than this")
parser.add_argument("--duration", type=float, default=-1.0,
                    help="Skip lines with the time column bigger than this.")
parser.add_argument("--time_column", type=int, default=1,
                    help="Which column contains the experiment time, 1-based indexing")
parser.add_argument("--x_column", type=int, default=1,
                    help="Which column contains x values, 1-based indexing")
parser.add_argument("--y_column", type=int, default=2,
                    help="Which column contains the y values, 1-based indexing")
parser.add_argument("--bin_size", type=float, default=1.0,
                    help="Bin duration")
parser.add_argument("--quantile", type=float, default=0.95,
                    help="Quantile to plot, used only with --type=quantile")
parser.add_argument("--bin_print", type=int, default=0,
                    help="Index of the bin to print values, used only with --type=print")
parser.add_argument("--metric", type=str, default="mean",
                    help="One of: mean, quantile, print")
args = parser.parse_args()

assert args.time_column >= 1
assert args.x_column >= 1
assert args.y_column >= 1
assert args.bin_size > 0
assert args.bin_print >= 0
assert args.metric in ['mean', 'quantile', 'print']

data_sum = []
data_all = []
for line in sys.stdin:
    tokens = line.split()
    assert len(tokens) >= max(args.x_column, args.y_column, args.time_column)
    time = float(tokens[args.time_column - 1])
    key = float(tokens[args.x_column - 1])
    value = float(tokens[args.y_column - 1])
    if time < args.warmup or (args.duration > 0 and time > args.duration):
        continue

    bin_ndx = int(round(key / args.bin_size))

    assert len(data_sum) == len(data_all)
    if bin_ndx >= len(data_sum):
        elems_to_add = 1 + bin_ndx - len(data_sum)
        data_sum.extend([0.0] * elems_to_add)
        for i in range(elems_to_add):
            data_all.append([])
    data_sum[bin_ndx] = data_sum[bin_ndx] + value
    data_all[bin_ndx].append(value)

for t,s,values in zip(range(len(data_sum)), data_sum, data_all):
    N = len(values)
    if N > 0:
        x = 0.0
        if args.metric == 'mean':
            x = s / N
        elif args.metric == 'quantile':
            values.sort()
            q = min(int(N * args.quantile), N-1)
            x = values[q]
        elif args.metric == 'print':
            if t == args.bin_print:
                for v in values:
                    print v
                break
            continue
        print '{} {}'.format(t * args.bin_size, x)

