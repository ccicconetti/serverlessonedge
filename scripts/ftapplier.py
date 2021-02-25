#!/usr/bin/python
"Apply forwarding table commands from a log"

import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--ovs_monitor_file", default="monitor.log",
                    help="ovs-monitor.py output file")
parser.add_argument("--forwarding_table_client", default='../bin/forwardingtableclient',
                    help="path of the command-line utility to modify edge routing tables")
parser.add_argument("server_address", nargs=1,
                    help="server address")
args = parser.parse_args()

f = subprocess.Popen(
    ['tail', '-F', args.ovs_monitor_file],
    stdout=subprocess.PIPE,stderr=subprocess.PIPE)

while True:
    line = f.stdout.readline()
    tokens = line.strip().split()

    if len(tokens) != 8 or (tokens[2] != 'enabling' and tokens[2] != 'disabling'):
        continue

    command = tokens[2]
    computer = tokens[4]
    router = tokens[7]

    if router != args.server_address[0]:
        continue

    if command == 'enabling':
        p = subprocess.Popen(
            [args.forwarding_table_client,
             '--action',
             'change',
             '--server-endpoint',
             '{}:6474'.format(router),
             '--destination',
             '{}:10000'.format(computer)],
            stdout=subprocess.PIPE)
    else:
        assert command == 'disabling'
        p = subprocess.Popen(
            [args.forwarding_table_client,
             '--action',
             'remove',
             '--server-endpoint',
             '{}:6474'.format(router),
             '--destination',
             '{}:10000'.format(computer)],
            stdout=subprocess.PIPE)

    for line in p.stdout:
        if 'OK' in line:
            print '{} command executed on {}'.format(command, router)
            continue

    print 'error issuing {} command to {}'.format(command, router)

