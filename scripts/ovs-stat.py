#!/usr/bin/python
"""Command-line tool to print at regular intervals
   the incremental number of bytes/packets of an OpenFlow switch"""

import datetime
import sys
import time
import argparse
import ovs

parser = argparse.ArgumentParser(
    description='Print aggregated throughput of OF switches',
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--switches", default='s1',
                    help="ovswitch names as a comma-separated list")
parser.add_argument("--output", default='',
                    help="output file, empty means stdout")
parser.add_argument("--verbose", action="store_true", default=False,
                    help="verbose output")
parser.add_argument("--interval", type=float, default=1,
                    help="interval between consecutive measurements")
parser.add_argument("--rate", action="store_true", default=False,
                    help="print rates instead of counters")
parser.add_argument("--metrics", default="bytes,pkts",
                    help="metrics to print")
parser.add_argument("--total", action="store_true", default=False,
                    help="add total over all switches")
args = parser.parse_args()

counters = {}
for sw in args.switches.split(','):
    counters[sw] = ovs.Counter(sw)

if len(counters) == 0:
    raise Exception("Specify at least one switch")

if 'bytes' not in args.metrics and 'pkts' not in args.metrics:
    raise Exception("Specify at least one metric to print")

handle = open(args.output, 'w') if args.output else sys.stdout

first = True
start_time = time.mktime(datetime.datetime.now().timetuple())
while True:
    now = time.mktime(datetime.datetime.now().timetuple())
    output = '{}'.format(now - start_time)
    tot_bytes = 0
    tot_pkts = 0
    for name in sorted(counters.keys()):
        counters[name].update()
        if args.verbose:
            counters[name].print_counters()
        norm = 1
        if args.rate:
            norm = args.interval
        output += ' {}'.format(name)
        if 'bytes' in args.metrics:
            output += ' {}'.format(counters[name].tbytes / norm)
        if 'pkts' in args.metrics:
            output += ' {}'.format(counters[name].tpkts / norm)
        tot_bytes = tot_bytes + counters[name].tbytes / norm
        tot_pkts = tot_pkts + counters[name].tpkts / norm
    if args.total:
        output += ' tot'
        if 'bytes' in args.metrics:
            output += ' {}'.format(tot_bytes)
        if 'pkts' in args.metrics:
            output += ' {}'.format(tot_pkts)
    output += '\n'
    if first:
        first = False
    else:
        handle.write(output)
        handle.flush()

    time.sleep(args.interval)

if handle is not sys.stdout:
    handle.close()
