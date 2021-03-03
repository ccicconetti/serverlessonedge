#!/usr/bin/env python
"""
Create a topology from geographical coordinates
"""

import argparse
import math


def distance(aNode, anotherNode):
    "Return the Euclidean distance between two nodes"
    return math.sqrt(
        (aNode[0] - anotherNode[0])**2 +
        (aNode[1] - anotherNode[1])**2)


parser = argparse.ArgumentParser(
    "Create a topology from geographical coordinates")
parser.add_argument("--in_file", type=str,
                    help="Input filename")
parser.add_argument("--topo_out", type=str,
                    help="Output topology filename")
parser.add_argument("--lines_out", type=str,
                    help="Output lines filename")
parser.add_argument("--out_file", type=str,
                    help="Filtered input after collapse")
parser.add_argument("--collapse_distance", type=float, default=0,
                    help="Maximum distance to collapse nodes")
parser.add_argument("--distance", type=float, default=0,
                    help="Minimum distance to create a link")
args = parser.parse_args()

assert args.in_file and args.out_file and args.topo_out and args.lines_out

coord = []
with open(args.in_file, 'r') as infile:
    n = 0
    for line in infile:
        n += 1
        node = line.rstrip().split()
        if len(node) != 2:
            raise Exception('Invalid input at line {}'.format(n))
        coord.append([float(node[0]), float(node[1])])
    print 'read coordinates of {} nodes'.format(n)

# remove nodes that are too close to one another
clean_coord = []
with open(args.out_file, 'w') as outfile:
    for nodea in coord:
        too_close = False
        for nodeb in coord:
            if nodea != nodeb and \
                    distance(nodea, nodeb) <= args.collapse_distance:
                too_close = True
                break
        if not too_close:
            outfile.write('{} {}\n'.format(nodea[0], nodea[1]))
            clean_coord.append(nodea)

print 'using {} nodes after collapse'.format(len(clean_coord))

with open(args.lines_out, 'w') as outlines,\
        open(args.topo_out, 'w') as outtopo:
    # save nodes first
    outtopo.write('Nodes: ({})\n'.format(len(clean_coord)))
    n = 0
    for node in clean_coord:
        outtopo.write('{} {} {}\n'.format(
            n,
            node[0],
            node[1]))
        n += 1
    outtopo.write('\n')

    edges = []
    a = 0
    n = 0
    for nodea in clean_coord:
        b = 0
        for nodeb in clean_coord:
            if a == b:
                continue
            d = distance(nodea, nodeb)
            if d <= args.distance:
                edges.append([n, a, b])
                # print '{} {} {}'.format(a, b, distance)
                outlines.write('{} {} {} {}\n'.format(
                    nodea[0],
                    nodea[1],
                    nodeb[0],
                    nodeb[1]))
                n += 1
            b += 1
        a += 1

    # save edges last
    outtopo.write('Edges: ({})\n'.format(len(edges)))
    for e in edges:
        outtopo.write('{} {} {}\n'.format(
            e[0],
            e[1],
            e[2]))
