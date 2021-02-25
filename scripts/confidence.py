#!/usr/bin/python

import argparse
import os
import statsmodels.stats.api as sms

parser = argparse.ArgumentParser("Compute mean and confidence interval from population of data over multiple files")
parser.add_argument("--xcol", type=int, default=1,
                    help="Which column to use for the x-axis, 1-based indexing; if 0 use line counter")
parser.add_argument("--ycol", type=int, default=2,
                    help="Which column to use for the y-axis, 1-based indexing")
parser.add_argument("--alpha", type=float, default=0.05,
                    help="Confidence level")
parser.add_argument("infile", nargs="*", help="input files")
args = parser.parse_args()

assert 0 < args.alpha < 1
assert args.xcol >= 0
assert args.ycol >= 1
assert len(args.infile) > 0

# check that all input files exist
for i in args.infile:
    if not os.path.isfile(i):
        print("Input file does not exist: " + i)
        exit(1)

# read all input files
data = dict()
for i in args.infile:
    f = open(i, 'r')
    cnt = 0
    for line in f:
        tokens = line.strip().split()
        if len(tokens) < max(args.xcol, args.ycol):
            print("Invalid line {} when reading from file: {}".format(1 + cnt, i))
            exit(1)
        x = cnt if args.xcol == 0 else float(tokens[args.xcol - 1])
        y = float(tokens[args.ycol - 1])
        if x not in data:
            data[x] = []
        data[x].append(y)
        #print ("{} [{}]: {} {}".format(i, cnt + 1, x, y))
        cnt = cnt + 1
    f.close()

# compute confidence intervals
for x,values in sorted(data.iteritems()):
    if len(values) >= 2:
        stat = sms.DescrStatsW(values)
        ci = stat.tconfint_mean(alpha = args.alpha)
        print("{} {} {} {}".format(x, stat.mean, ci[0], ci[1]))
    else:
        print("{} {} {} {}".format(x, values[0], values[0], values[0]))

exit()
