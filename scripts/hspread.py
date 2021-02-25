#!/usr/bin/python
"""Compute the average inter-quantile between multiple data sets"""

#import math
import argparse

parser = argparse.ArgumentParser(
    "Compute the average inter-quantile",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--warmup", type=float, default=0,
                    help="Skip lines with the warm-up column small than this")
parser.add_argument("--duration", type=float, default=-1,
                    help="Skip lines with the warm-up column bigger than this")
parser.add_argument("--time_bin", type=float, default=1,
                    help="Time aggregation bin")
parser.add_argument("--time_column", type=int, default=1,
                    help="Which column contains the experiment time, " +\
                          "1-based indexing; 0 means line counter")
parser.add_argument("--data_column", type=int, default=2,
                    help="Which column contains the data, 1-based indexing")
parser.add_argument("input", type=str, nargs='*',
                    help="Files containing the data")
args = parser.parse_args()

assert args.data_column > 0
assert args.time_bin > 0
assert len(args.input) >= 2

data = dict()
for filename in args.input:
    with open(filename, 'r') as infile:
        for line in infile:
            tokens = line.rstrip().split()
            assert len(tokens) >= max(args.time_column, args.data_column)

            time = float(tokens[args.time_column - 1])
            value = float(tokens[args.data_column - 1])

            if time < args.warmup or (args.duration > 0 and time >= args.duration):
                continue

            bin_ndx = int(round(time / args.time_bin))

            if filename not in data.keys():
                data[filename] = dict()
            this_data = data[filename]

            if bin_ndx not in this_data.keys():
                this_data[bin_ndx] = []
            this_data[bin_ndx].append(value)

averages = dict()
for filename,all_data in data.items():
    for bin_ndx,values in all_data.items():
        if bin_ndx not in averages.keys():
            averages[bin_ndx] = []
        sum_values = 0.0
        for v in values:
            sum_values += v
        averages[bin_ndx].append(sum_values / len(values))

for bin_ndx,values in averages.items():
    q25_ndx = int(round(float(len(values)) * 0.25))
    q75_ndx = int(round(float(len(values)) * 0.75))
    assert q25_ndx != q75_ndx
    assert q25_ndx > 0
    assert len(values) >= q75_ndx

    sorted_values = sorted(values)
    print '{} {}'.format(
        bin_ndx * args.time_bin,
        sorted_values[q75_ndx - 1] - sorted_values[q25_ndx - 1])
