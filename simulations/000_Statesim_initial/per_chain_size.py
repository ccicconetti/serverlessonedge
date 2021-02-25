#!/usr/bin/env python3

import sys
import statsmodels.stats.api as sms

data = dict()
for line in sys.stdin:
    line.rstrip()
    tokens = line.split(" ")
    k = int(tokens[3])
    if k not in data:
        data[k] = [[], [], []]
    for i in range(3):
        data[k][i].append(float(tokens[i]))

multipliers = [1e3, 1e3, 1e-6]
names = ["proclat", "netlat", "traffic"]

for i in range(3):
    with open(f"{names[i]}.dat", "w") as outfile:
        for k in sorted(data.keys()):
            vector = data[k][i]
            if len(vector) <= 2:
                continue
            [low, high] = sms.DescrStatsW(vector).tconfint_mean(alpha=0.05)
            mean = (low + high) / 2
            outfile.write(f"{k} {mean} {low} {high}\n")
