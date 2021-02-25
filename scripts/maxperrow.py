#!/usr/bin/python
"""Print the maximum over columns"""

import sys
import argparse

parser = argparse.ArgumentParser(
    description='For each row print the maximum over columns',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--cols", type=int, nargs='*',
                    help="columns to be used, 1-based")
args = parser.parse_args()

assert args.cols
assert len(args.cols) > 0

max_cols = max(args.cols)
assert max_cols >= 1

for line in sys.stdin:
    tokens = line.rstrip().split()
    assert len(tokens) >= max_cols
    values = []
    for col in args.cols:
        values.append(float(tokens[col-1]))
    print max(values)

