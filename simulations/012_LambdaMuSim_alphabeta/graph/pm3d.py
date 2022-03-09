#!/usr/bin/env python3
"""
Compute summary statistics from a dataset on a file
"""

import argparse
import sys

parser = argparse.ArgumentParser(
    "Filter lambdamusim output for use with pm3d plot in gnuplot",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--lambda_value", type=int, default=0,
                    help="Number of lambda apps")
parser.add_argument("--mu_value", type=int, default=0,
                    help="Number of mu apps")
parser.add_argument("--column", type=int, default=1,
                    help="Column to print (1-based)")
args = parser.parse_args()

# 1-based indexing
lambda_col = 6
mu_col = 7
alpha_col = 8
beta_col = 9

data = dict()
for line in sys.stdin:
    line = line.rstrip()
    tokens = line.split(',')

    if max(lambda_col, mu_col, alpha_col, beta_col, args.column) > len(tokens):
        raise RuntimeError("Invalid line: " + line)

    # print(f'{args.lambda_value} {args.mu_value} {tokens[lambda_col-1]} {tokens[mu_col-1]}')
    if int(tokens[lambda_col-1]) != args.lambda_value or int(tokens[mu_col-1]) != args.mu_value:
        continue

    alpha = float(tokens[alpha_col-1])
    beta = float(tokens[beta_col-1])
    if alpha not in data:
        data[alpha] = dict()
    if beta not in data[alpha]:
        data[alpha][beta] = [0.0, 0]
    data[alpha][beta][0] += float(tokens[args.column-1])
    data[alpha][beta][1] += 1

for alpha, value in data.items():
    for beta, (sum, num) in value.items():
        print(f'{alpha} {beta} {sum /num}')
    print('')
