"""Library of Mininet topologies for edge computing"""

from mininet.topo import Topo
from mininet.nodelib import NAT

class SingleSwitchTopo(Topo):
    "Single switch connected to n hosts."

    def __init__(self, n, args):
        assert n >= 1

        # Initialize topology
        Topo.__init__(self)

        # create the single switch
        switch = self.addSwitch('s1')

        # create the hosts, each connect to a switch
        for h in range(n):
            host = self.addHost('h%s' % (h + 1))
            self.addLink(host, switch, **args)

class BalancedTreeTopo(Topo):
    """Balance tree topology. Each switch is connected to one host, then the leaves also
       have an additional order of hosts to model the access network with clients."""

    def __init__(self, order, depth, bw_lat):
        """Constructor

        Args:
            order: number of children switches per parent;
            depth: depth of the tree: 1 means single node;
            bw_lat: is a container of pairs that specifies for each level
                    the bandwidth and latency of the links connecting to
                    the nodes in the level below, starting from the root
        """
        assert order > 1
        assert depth > 1
        assert len(bw_lat) == depth

        # Initialize topology
        Topo.__init__(self)

        # number of leaf switches
        n_leaves = order ** (depth - 1)

        # total number of switches
        n_nodes = (1 - order ** depth) / (1 - order)

        # number of hosts:
        # + one per switch
        # + a further layer at the bottom
        n_hosts = n_nodes + order ** depth

        print '{} switches ({} leaves), {} hosts'.format(n_nodes, n_leaves, n_hosts)

        # crete the hosts
        hosts = [self.addHost('h%s' % (h + 1))
                 for h in range(n_hosts)]

        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n_nodes)]

        # create the links between switches
        cur_node = 0
        for level in range(depth - 1):
            cur_child = cur_node + order ** level
            for parent in range(order ** level):
                for child in range(order):
                    print 'level {} parent {} child {}: {} -> {} (bw = {}, lat = {})'.format(
                        level,
                        parent,
                        child,
                        cur_node,
                        cur_child,
                        bw_lat[level][0],
                        bw_lat[level][1])
                    self.addLink(switches[cur_node],
                                 switches[cur_child],
                                 bw=bw_lat[level][0],
                                 delay=bw_lat[level][1],
                                 loss=0,
                                 max_queue_size=300,
                                 use_htb=True)
                    cur_child += 1
                cur_node += 1
        assert cur_node == (n_nodes - n_leaves)

        # create the links between switches and their hosts
        for i in range(n_nodes):
            self.addLink(switches[i], hosts[i])

        # create the links between leaf switches and the access-network hosts
        for switch in range(n_nodes - n_leaves, n_nodes):
            for j in range(order):
                print 'level {} parent {} child {}: {} -> {} (bw = {}, lat = {})'.format(
                    depth - 1,
                    switch,
                    j,
                    switch,
                    n_nodes + (switch - n_nodes + n_leaves) * order + j,
                    bw_lat[depth-1][0],
                    bw_lat[depth-1][1])
                self.addLink(switches[switch],
                             hosts[n_nodes + (switch - n_nodes + n_leaves) * order + j],
                             bw=bw_lat[depth-1][0],
                             delay=bw_lat[depth-1][1],
                             loss=0,
                             max_queue_size=300,
                             use_htb=True)

class NestedCircularTopo(Topo):
    "One ring of inner switches, each connected point-to-point to a number of outer switches"

    def __init__(self, n_inner, n_outer, linkargs, hostargs):
        assert n_inner >= 1
        assert n_outer >= 0

        # Initialize topology
        Topo.__init__(self)

        # total number of switches
        n_tot = n_inner * (1 + n_outer)

        # crete the hosts
        hosts = [self.addHost('h%s' % (h + 1), **hostargs)
                 for h in range(n_tot)]
        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n_tot)]

        # connect the switches in the inner ring
        for i in range(n_inner):
            self.addLink(switches[i], switches[(i+1)%n_inner], **linkargs)

        # connect the switches in the outer ring
        for i in range(n_inner):
            for j in range(n_outer):
                self.addLink(switches[n_inner + n_outer * i + j], switches[i], **linkargs)

        # create the hosts: one per switch
        for host, switch in zip(hosts, switches):
            self.addLink(host, switch)

class ThreeTierTopo(Topo):
    "Bottom tier: clients; middle tier: edge routers/dispatchers; top tier: computers"

    def __init__(self, n_cpr, n_routers, bw_lat):
        """Constructor

        Args:
            n_cpr: is the number of clients per router;
            n_routers: is the number of nodes in the middle tier acting as edge routers;
            bw_lat: is a container of pairs that specifies for each node in the top tier,
                ie. edge computers, the bandwidth and latency of the links connecting it to
                the nodes in the middle tier
        """

        assert n_cpr >= 1
        assert n_routers >= 1
        assert len(bw_lat) >= 1

        # Initialize topology
        Topo.__init__(self)

        # total number of switches
        n_tot = (1 + n_cpr) * n_routers + len(bw_lat)
        print n_tot

        # crete the hosts
        hosts = [self.addHost('h%s' % (h + 1))
                 for h in range(n_tot)]
        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n_tot)]

        # let C = n_cpr
        #     R = n_routers
        #     S = len(bw_lat)
        #
        # numbering:
        # 0 .. C*R             -> clients
        # C*R .. (C+1)*R       -> routers
        # (C+1)*R .. (C+1)*R+S -> computers

        # connect the switches between the bottom and middle tiers
        for i in range(n_cpr * n_routers):
            self.addLink(switches[i], switches[n_cpr * n_routers + (i / n_cpr)])

        # connect the switches between the middle and top tiers
        for r in range(n_cpr * n_routers, (1+n_cpr) * n_routers):
            s = (1+n_cpr) * n_routers
            print "*** {} {}".format(r, s)
            for bw,lat in bw_lat:
                self.addLink(switches[r],
                             switches[s],
                             bw=bw, delay=lat, loss=0, max_queue_size=300, use_htb=True)
                s = s + 1

        # create the hosts: one per switch
        for host, switch in zip(hosts, switches):
            self.addLink(host, switch)

class TwoTierTopo(Topo):
    "Bottom tier: clients; top tier: edge nodes connected in a clique"

    def __init__(self, n_cpr, n_routers, bw_lat_clients, bw_lat_routers):
        """Constructor

        Args:
            n_cpr: is the number of clients per router;
            n_routers: is the number of edge nodes == routers;
            bw_lat_clients: a pair of (bw, latency) specifying the
                link between clients and routers;
            bw_lat_routers: a pair of (bw, latency) specifying the
                link between routers.
        """

        assert n_cpr >= 1
        assert n_routers >= 1
        assert len(bw_lat_clients) == 2
        assert len(bw_lat_routers) == 2

        # Initialize topology
        Topo.__init__(self)

        # crete the hosts
        hosts = [self.addHost('h%s' % (h + 1))
                 for h in range((1 + n_cpr) * n_routers)]
        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n_routers)]

        # add one host to each edge node
        for i in range(n_routers):
            self.addLink(hosts[i], switches[i])

        # connect the routers in a clique topology
        for i in range(n_routers):
            for j in range(i):
                self.addLink(
                    switches[i],
                    switches[j],
                    bw=bw_lat_routers[0],
                    delay=bw_lat_routers[1],
                    loss=0, max_queue_size=300, use_htb=True)

        # connect the clients to edge nodes
        for i in range(n_routers):
            for j in range(n_cpr):
                self.addLink(
                    switches[i],
                    hosts[n_routers + i * n_cpr + j],
                    bw=bw_lat_clients[0],
                    delay=bw_lat_clients[1],
                    loss=0, max_queue_size=300, use_htb=True)

class HourglassTopo(Topo):
    "N nodes in the top tier separated from M nodes in the bottom tier by a single node"

    def __init__(self, n_upper, n_lower, bw_lat_upper, bw_lat_middle, bw_lat_lower):
        """Constructor

        Args:
            n_upper: the number of nodes in the upper tier
            n_lower: the number of nodes in the lower tier
            bw_lat_upper: a pair of (bw, latency) specifying the
                characteristics of links in the upper tier
            bw_lat_middle: a pair of (bw, latency) specifying the
                characteristics of the link between tiers
            bw_lat_lower: a pair of (bw, latency) specifying the
                characteristics of links in the lower tier
        """

        assert n_upper >= 1
        assert n_lower >= 1
        assert len(bw_lat_upper) == 2
        assert len(bw_lat_lower) == 2

        # Initialize topology
        Topo.__init__(self)

        # crete the hosts
        hosts = [self.addHost(name= 'h%s' % (h + 1))
                 for h in range(n_upper + n_lower)]

        # create the switches
        switches = [self.addSwitch('s1'), self.addSwitch('s2')]

        # connect the two switches
        self.addLink(
            switches[0],
            switches[1],
            bw=bw_lat_middle[0],
            delay=bw_lat_middle[1],
            loss=0, max_queue_size=300, use_htb=True)

        # connect the upper hosts
        for i in range(n_upper):
            print "{} {}".format(switches[0], hosts[i])
            self.addLink(
                switches[0],
                hosts[i],
                bw=bw_lat_upper[0],
                delay=bw_lat_upper[1],
                loss=0, max_queue_size=300, use_htb=True)

        # connect the lower hosts
        for i in range(n_lower):
            print "{} {}".format(switches[1], hosts[n_upper + i])
            self.addLink(
                switches[1],
                hosts[n_upper + i],
                bw=bw_lat_lower[0],
                delay=bw_lat_lower[1],
                loss=0, max_queue_size=300, use_htb=True)

class StarTopo(Topo):
    "A single switch (with host) connected n switches, each connecting m + 1 hosts"

    def __init__(self, n_cpr, n_routers, bw_lat_clients, bw_lat_routers, natted = False):
        """Constructor

        Args:
            n_cpr: is the number of clients per router (m);
            n_routers: is the number of switches connected to the
                root (n);
            bw_lat_clients: a pair of (bw, latency) specifying the
                link between clients and routers;
            bw_lat_routers: a pair of (bw, latency) specifying the
                link between routers.
        """

        assert n_cpr >= 1
        assert n_routers >= 1
        assert len(bw_lat_clients) == 2
        assert len(bw_lat_routers) == 2

        # Initialize topology
        Topo.__init__(self)

        # crete the hosts
        hosts = []
        for i in range(1 + n_routers):
            if natted:
                self.hopts = { 'defaultRoute': 'via ' + '10.1.0.254' }
            hosts.append(self.addHost( 'h%s' % ( i + 1) ))

        self.hopts = dict()
        for i in range(1 + n_routers, 1 + (1 + n_cpr) * n_routers, 1):
            hosts.append(self.addHost( 'h%s' % ( i + 1) ))

        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(1 + n_routers)]

        # add one host to each switch
        for i in range(1 + n_routers):
            self.addLink(hosts[i], switches[i])

        # connect the star switches to the root
        for i in range(1, 1 + n_routers):
            self.addLink(
                switches[0],
                switches[i],
                bw=bw_lat_routers[0],
                delay=bw_lat_routers[1],
                loss=0, max_queue_size=300, use_htb=True)

        # connect the clients to the star switches
        for i in range(n_routers):
            for j in range(n_cpr):
                self.addLink(
                    switches[1 + i],
                    hosts[1 + n_routers + i * n_cpr + j],
                    bw=bw_lat_clients[0],
                    delay=bw_lat_clients[1],
                    loss=0, max_queue_size=300, use_htb=True)

        # Create the NAT'ed nodes
        if natted:
            # create the nat node
            nat_host = self.addHost( 'h%s' % (1 + len(hosts) ),
                cls=NAT, ip='10.1.0.254', inNamespace=False )

            # create the nat switch
            nat_switch = self.addSwitch('s%s' % (2 + n_routers))

            self.addLink(nat_switch, nat_host)

            # create links
            for i in range(1 + n_routers):
                self.addLink(nat_switch, switches[i])


class LinearTopo(Topo):
    """
    Topology for a string of N hosts and N switches.

    Parameters:
    ----------
    n:          int
                Number of hosts.

    linkargs:   dict
                Arguments to create links.

    hostargs:   dict
                Arguments to create hosts.
    """

    def __init__(self, n, linkargs, hostargs):
        assert n >= 1

        # Initialize topology
        Topo.__init__(self)

        # Create switches and hosts
        hosts = [self.addHost('h%s' % (h + 1), **hostargs)
                 for h in range(n)]
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n)]

        # Wire up switches
        last = None
        for switch in switches:
            if last:
                self.addLink(last, switch, **linkargs)
            last = switch

        # Wire up hosts
        for host, switch in zip(hosts, switches):
            self.addLink(host, switch)


#
# Example 2x3
#
#  1 -- 2
#  |    |
#  3 -- 4
#  |    |
#  5 -- 6
#
class GridTopo(Topo):
    "Topology for a grid of NxM switches, each with a host connected"

    def __init__(self, n, m, natted_nodes, args):
        assert n >= 1
        assert m >= 1

        # Initialize topology
        Topo.__init__(self)

        # Create switches and hosts
        hosts = []
        for i in range(n * m):
            if i in natted_nodes:
                self.hopts = { 'defaultRoute': 'via ' + '10.1.0.254' }
                hosts.append(self.addHost( 'h%s' % ( i + 1) ))
            else:
                self.hopts = dict()
                hosts.append(self.addHost( 'h%s' % ( i + 1) ))
        switches = [ self.addSwitch( 's%s' % ( s + 1) )
                     for s in range(n * m) ]

        # Wire up switches
        for i in range(m):
            for j in range(n):
                ndx = i * n + j
                cur = switches[ndx]

                if i > 0:        # above
                    self.addLink( cur, switches[ndx - n], **args )
                if i < (m - 1):  # below
                    self.addLink( cur, switches[ndx + n], **args )
                if j > 0:        # left
                    self.addLink( cur, switches[ndx - 1], **args )
                if j < (n - 1):  # right
                    self.addLink( cur, switches[ndx + 1], **args )

        # Wire up hosts
        for host, switch in zip( hosts, switches ):
            self.addLink( host, switch )

        # Create the NAT'ed nodes
        if natted_nodes:
            # create another switch for the nat
            nat_switch = self.addSwitch( 's%s' % (1 + n * m) )

            # create the nat node
            nat_host = self.addHost( 'h%s' % (1 + n * m),
                cls=NAT, ip='10.1.0.254', inNamespace=False )

            self.addLink(nat_switch, nat_host)

            # create link with all the natted hosts
            for i in natted_nodes:
                self.addLink(nat_switch, switches[i])

#
# Example with n = 4 (20 switches):
#
# 003 --- 002                     014 --- 015
#  |       |                       |       |
#  |       |                       |       |
# 004 --- 001 --- 017 --- 020 --- 013 --- 016
#                  |       |
#                  |       |
# 006 --- 005 --- 018 --- 019 --- 009 --- 011
#  |       |                       |       |
#  |       |                       |       |
# 007 --- 008                     010 --- 011
#
class PodsTopo(Topo):
    "Topology for a two-tier pod-based structure"

    def __init__(self, n, args):
        assert n >= 1

        # Initialize topology
        Topo.__init__(self)

        # Create switches and hosts
        hosts = [self.addHost('h%s' % (h + 1))
                 for h in range(n * (n + 1))]
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(n * (n + 1))]

        # Wire up switches within each pod, including the middle one
        for i in range(0, len(switches), n):
            for j in range(n):
                self.addLink( switches[i + j], switches[i + (j + 1) % n], **args )

        # Wire up edge switches to the middle pod
        for i, j in zip(range(0, n * n, n), range(n * n, len(switches), 1)):
            self.addLink( switches[i], switches[j], **args )

        # Wire up hosts
        for host, switch in zip( hosts, switches ):
            self.addLink( host, switch )

class BriteTopo(Topo):
    "Load topology from Brite file"

    def __init__(self, verbose, filename, args):
        # Initialize topology
        Topo.__init__(self)

        # Save graphviz
        out_file = open('{}.dot'.format(filename), 'w')
        print >>out_file, 'graph G {'

        # Read from file
        in_file = open(filename, 'r')

        switches = []
        edge_mode = False
        node_mode = False
        cnt=0
        for line in in_file:
            cnt = cnt + 1
            if 'Nodes:' in line:
                node_mode = True
                continue
            elif 'Edges:' in line:
                edge_mode = True
                node_mode = False
                continue

            if node_mode:
                if len(line.lstrip()) == 0:
                    node_mode = False
                    continue
                node_id = 1 + int(line.split(' ')[0])
                new_host = self.addHost('h{}'.format(node_id))
                new_switch = self.addSwitch('s{}'.format(node_id))
                self.addLink(new_host, new_switch)
                switches.append(new_switch)
                #print >>out_file, '"h{}" -- "s{}"'.format(node_id, node_id)

            if edge_mode:
                tokens = line.split(' ')
                link_id = int(tokens[0])
                from_node = int(tokens[1])
                to_node = int(tokens[2])
                if verbose:
                    print '{} : {} - {}'.format(link_id, from_node, to_node)
                self.addLink(switches[from_node], switches[to_node], **args)
                print >>out_file, '"s{}" -- "s{}"'.format(from_node + 1, to_node + 1)

        print >>out_file, '}'

        in_file.close()
        out_file.close()
