#!/usr/bin/python
"""Command-line tool to monitor at regular intervals the rate on
   every link: if it goes above a given threshold, all the edge
   computers affected are removed from the forwarding tables of
   relevant edge routers. The entries are then restored once the
   rate goes below the threshold again."""

import logging
import time
import argparse
import json

import ftu
import ovs

parser = argparse.ArgumentParser(
    description='Monitor OF switches are react accordinly on edge routers')
parser.add_argument("--conf", default='conf.json',
                    help="configuration file")
parser.add_argument("--interval", type=float, default=1,
                    help="interval between consecutive measurements")
parser.add_argument("--threshold", type=float, default=1e6,
                    help="threshold above which to trigger actions, in b/s")
parser.add_argument("--forwarding_table_client", default='../bin/forwardingtableclient',
                    help="path of the command-line utility to modify edge routing tables")
parser.add_argument("--dry_run", action='store_true', default=False,
                    help="do not run the command")
parser.add_argument("--loglevel", default='warning',
                    help="logging level")
args = parser.parse_args()

assert args.threshold > 0
assert args.interval > 0

# configure logging
numeric_level = getattr(logging, args.loglevel.upper(), None)

if not isinstance(numeric_level, int):
    raise ValueError('Invalid log level: %s' % args.loglevel)
logging.basicConfig(level=numeric_level, format='%(asctime)s %(message)s')

# read configuration file
with open(args.conf) as f:
    conf = json.load(f)

assert len(conf['switches']) > 0
assert len(conf['computers']) > 0
assert len(conf['routers']) > 0

# create counters
counters = {}
for sw in conf['switches']:
    counters[sw] = ovs.Counter(sw)

# normalized threshold, in bytes
threshold = args.threshold / 8 / args.interval

# create forwarding table update
ftu = ftu.ForwardingTableUpdater(
    conf['computers'],
    conf['routers'],
    conf['affected_paths'],
    args.forwarding_table_client,
    args.dry_run)

# main loop
while True:
    cong_links = []
    for name in sorted(counters.keys()):
        counters[name].update()
        congested = counters[name].above_threshold(threshold)
        for c in congested:
            cong_links.append('{},{}'.format(name, c))
    if len(cong_links) > 0:
        logging.info(cong_links)
    ftu.update(cong_links)
    time.sleep(args.interval)
