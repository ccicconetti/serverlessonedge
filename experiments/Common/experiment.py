#!/usr/bin/python
"""Class to execute a full Mininet edge computing experiment"""

import argparse
import json
import os
import random
import time
import subprocess

from mininet.net import Mininet
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.cli import CLI
from mininet.node import OVSKernelSwitch
from mininet.link import TCLink
#from mininet.node import OVSSwitch
from mininet.node import Host, RemoteController, Switch, Controller, CPULimitedHost
from mininet.nodelib import NAT

import cmdrunner
import dijkstra


class Experiment(object):
    "Run a mininet experiment"

    def __init__(self, **kwargs):
        self.confopts = kwargs

        print 'Command-line arguments:'
        for key, value in kwargs.iteritems():
            print '{}: {}'.format(key, value)

        experiment_label = 'generic'
        if 'edgetype' in self.confopts and 'balancing' in self.confopts:
            assert 'dispatchertype' in self.confopts
            if self.confopts['edgetype'] == 'router':
                experiment_label = self.confopts['balancing']
            elif self.confopts['edgetype'] == 'dispatcher':
                experiment_label = self.confopts['dispatchertype']
            else:
                raise Exception('Invalid --edgetype value: ' +
                                self.confopts['edgetype'])
        elif 'edgetype' in self.confopts:
            assert 'dispatchertype' in self.confopts
            assert 'balancing' not in self.confopts
            experiment_label = self.confopts['dispatchertype']
        elif 'balancing' in self.confopts:
            assert 'edgetype' not in self.confopts
            experiment_label = self.confopts['balancing']

        # initialize pseudo-random number generation
        random.seed(self.confopts['seed'])

        # the mininet emulated network
        self.net = None

        # an identifier of the experiment, to be enriched by derived classes
        self.experiment_id = '{}.{}.{}'.format(
            self.confopts['controller'],
            experiment_label,
            self.confopts['seed'])

        # instance of CmdRunner to execute commands in emulated hosts
        self.cmd = cmdrunner.CmdRunner(self.confopts['verbosity'] >= 2)

        # list of hosts in the network
        self.hosts = []

        # list of switches in the network
        self.switches = []

        # list of natted hosts in the network
        self.nathosts = []

        # IP address of the NAT
        self.nat_ip = '10.1.0.254'

        # destination network to reach via NAT
        self.nat_network = '192.168.122.0/24'

        # string added to the command-line of edge computers and routers
        self.controller_conf = ''

        # list of hosts that are acting as routers
        self.routers = []

        # hash
        # key:   host on which there is a computer
        # value: list of tuples indicating:
        #        #0: the lambda processor's end-point
        #        #1: the utilization collector's end-point
        #        #2: the list of lambdas offered
        self.computers = {}

        # network topology
        self.graph = {}

        self.itgrecv = []

        # an object representing the configuration of the experiment intended
        # to be used to serialize to a JSON file
        self.jconf = {}

        setLogLevel(kwargs['loglevel'])

    @staticmethod
    def makeArgumentParser(script_args, blacklisted):
        """
        Make a namespace from parsing command the command line arguments.

        Parameters
        ----------

        stript_args: hash
                     Map of additional arguments, where the key is the name of
                     the argument and the value is either a pair of default value
                     and help string (in which case it is assumed that the argument
                     is a Boolean) or a triple of type, default value, and help string.
        blacklisted: hash
                     Map of argument names and default values.
                     All the arguments in this hash are not exposed via the cli.

        Returns
        ----------
        namespace
                    Parsed arguments' namespace
        """

        parser = argparse.ArgumentParser(
            "Run edge computing mininet experiment",
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)

        # key:   name of the command-line argument
        # value: pair of <default, help> if the argument is a Boolean
        #        triple of <type, default, help> otherwise
        arguments = {
            'balancing': [
                str,
                'rr-async',
                'one of {rr-async,li-async,rp-async,rr-periodic,li-periodic,rp-periodic}'],
            'cli': [
                int,
                4,
                'enable the CLI after the given phase'],
            'controller': [
                str,
                'flat',
                'one of {none, flat, hier}'],
            'cpu_limit': [
                float,
                -1.0,
                'CPU limit, negative means unlimited'],
            'dispatchertype': [
                str,
                'rtt',
                'dispatcher type: one of {rtt, util, util-opt, probe, none}'],
            'duration': [
                int,
                10,
                'experiment duration, in s'],
            'edgetype': [
                str,
                'router',
                'one of {dispatcher,router}'],
            'loglevel': [
                str,
                'info',
                'mininet logging level'],
            'nat': [
                False,
                'use NAT to access the emulated network from outside'],
            'save_topo': [
                str,
                '',
                'save topology graph to the specified file'],
            'seed': [
                int,
                42,
                'pseudo-random seed generator'],
            'tclink': [
                str,
                'wotc',
                'link type, one of {wotc, wtc2}']
        }

        # merge default arguments with user-provided ones
        arguments.update(script_args)
        for argvar, argvalues in arguments.items():
            assert 2 <= len(argvalues) <= 3
            if argvar in blacklisted:
                continue
            if len(argvalues) == 2:
                parser.add_argument(
                    '--' + argvar,
                    action="store_true",
                    default=argvalues[0],
                    help=argvalues[1])
            else:
                parser.add_argument(
                    '--' + argvar,
                    type=argvalues[0],
                    default=argvalues[1],
                    help=argvalues[2])

        # mandatory arguments cannot be blacklisted)
        parser.add_argument("-v", "--verbosity", action="count", default=0,
                            help="increase output verbosity")

        # parse command-line arguments
        args = parser.parse_args()

        # load default values of blacklisted arguments
        args.__dict__.update(blacklisted)

        return args

    @staticmethod
    def makeComputerJson(
            outfile,
            lambdas,
            speed=1e9,
            num_cores=4,
            op_coeff=1e6,
            op_offset=4e6,
            mem_coeff=100,
            mem_offset=0):
        "Save to file an edge computer template"

        assert num_cores > 0
        assert len(lambdas) > 0

        computer_file = open(outfile, 'w')
        processor_json = dict()
        processor_json['name'] = 'arm'
        processor_json['type'] = 'GenericCpu'
        processor_json['speed'] = speed
        processor_json['memory'] = 512*1024*1024
        processor_json['cores'] = int(num_cores)
        containers = []
        for lambda_name, i in zip(lambdas, range(len(lambdas))):
            lambda_json = dict()
            lambda_json['name'] = lambda_name
            lambda_json['output-type'] = 'copy-input'
            lambda_json['requirements'] = 'proportional'
            lambda_json['op-coeff'] = op_coeff
            lambda_json['op-offset'] = op_offset
            lambda_json['mem-coeff'] = mem_coeff
            lambda_json['mem-offset'] = mem_offset
            container_json = dict()
            container_json['name'] = 'ccontainer{}'.format(i)
            container_json['processor'] = 'arm'
            container_json['lambda'] = lambda_json
            container_json['num-workers'] = int(num_cores)
            containers.append(container_json)
        computer_json = dict()
        computer_json['version'] = '1.0'
        computer_json['processors'] = [processor_json]
        computer_json['containers'] = containers
        computer_file.write(json.dumps(computer_json))
        computer_file.close()

    def backgroundTraffic(self, send, recv, rate, start, duration):
        "Create background traffic"

        pkt_size = 1500  # bytes
        if recv not in self.itgrecv:
            self.cmd.run(recv,
                         '../bin/ITGRecv',
                         is_background=True)
            time.sleep(1)

        self.cmd.run(send,
                     '../bin/ITGSend -T UDP -a {} -c {} -C {} -t {} -d {}'.format(
                         recv.IP(),
                         pkt_size,
                         rate / pkt_size / 8,
                         duration * 1000,
                         start * 1000),
                     is_background=True)

    def createEdgeController(self):
        "Create an instance of edge controller, if required"

        if self.confopts['controller'] == 'none':
            return

        print "Starting edge controller"

        if 'flat' in self.confopts['controller']:
            self.cmd.run(self.hosts[0],
                         ('../bin/edgecontroller '
                          '--type plain '
                          '--installer-type flat'),
                         is_background=True)
        else:
            assert 'hier' in self.confopts['controller']
            assert os.path.isfile('dist.txt')
            hier_objective = 'min-max'  # default if objective not specified
            topology_randomized = ''    # default is NOT to randomize topology
            if 'min-avg' in self.confopts['controller']:
                hier_objective = 'min-avg'
            if 'random' in self.confopts['controller']:
                topology_randomized = '--topology-randomized'
            self.cmd.run(self.hosts[0],
                         ('../bin/edgecontroller '
                          '--type plain '
                          '--installer-type hier '
                          '--hier-objective {} '
                          '{} '
                          '--topology-file dist.txt').format(
                              hier_objective,
                              topology_randomized),
                         is_background=True)

        self.controller_conf = "--controller {}:6475".format(
            self.hosts[0].IP())
        self.jconf['controller'] = self.hosts[0].IP()
        time.sleep(1)

    def createUtilizationCollectors(self):
        "Create instances of utilization collectors, one for each computer"

        # sleep a bit to make sure the edge computers are active
        time.sleep(1)

        print "Starting utilization collectors"
        for c, endpoints in self.computers.items():
            assert len(endpoints) > 0
            for e in endpoints:
                assert len(e) >= 2

                if not e[1]:
                    # do not start the util collector if no end-point is specified
                    continue

                plotname = 'results/util.{}.{}'.format(
                    self.experiment_id, e[0])
                self.cmd.run(
                    c,
                    '../bin/edgecomputerclient --server {} --output {}'.format(
                        e[1],
                        plotname),
                    is_background=True)

    def waitForEdgeRoutersReady(self):
        "Wait for all edge routers to respond to a client"

        print "Waiting for all edge routers to be ready"
        for client in self.routers:
            if(self.confopts['experiment'] == 'grpc'):
                while ('Errors' in client.cmd(
                    '../bin/edgeclient --server-endpoint {}:6473 --lambda clambda0 --max-requests 1 --client-conf type=grpc,persistence=0.05'.format(
                        client.IP()))):
                    pass
            else:
                time.sleep(1)

            print 'router {} ready'.format(client.name)

    def saveForwardingTableSize(self):
        "Save the forwarding table size of e-routers"

        self.hosts[0].cmd('rm results/table.{}'.format(self.experiment_id))
        for client in self.routers:
            client.cmd(
                '../../scripts/table_size.sh {}:6474 >> results/table.{}'.format(
                    client.IP(),
                    self.experiment_id))

    def installForwardingTables(self):
        "Manually install forwarding tables into edge routers based on topology only"

        if self.confopts['controller'] != 'none':
            return

        print "Installing forwarding tables"
        serving_computers = {}
        computers_used = []
        for h in self.hosts:
            if h.name in [r.name for r in self.routers]:
                # find closest computer (in number of hops)
                closest = None
                cur_dist = 0
                for node, dist in dijkstra.spt(self.graph, 's{}'.format(h.name[1:]))[1].items():
                    if node not in [c.name for c in self.computers.keys()] \
                            or node in computers_used:
                        continue
                    if closest is None or dist < cur_dist:
                        closest = node
                        cur_dist = dist
                serving_computers[h.name] = closest
                computers_used.append(closest)

        for router, computer in serving_computers.items():
            for computer_tuple in self.computers[self.net.get(computer)]:
                if len(computer_tuple) < 3:
                    continue
                for lambda_name in computer_tuple[2]:
                    self.cmd.run(
                        self.net.get(router),
                        ('../bin/forwardingtableclient '
                         '--server {}:6474 '
                         '--action change '
                         '--final '
                         '--lambda {} '
                         '--destination {}').format(
                             self.net.get(router).IP(),
                             lambda_name,
                             computer_tuple[0]),
                        is_background=False)

        for router, computer in serving_computers.items():
            print 'router {} served by computer {}'.format(router, computer)
            if computer is None:
                raise Exception(
                    "There is no edge computer serving edge router {}".format(router))

    def printForwardingTables(self):
        "Print the forwarding tables of all the edge routers"

        if self.routers:
            print "Printing the current forwarding tables of all the edge routers"

        assert len(self.hosts) > 0
        for h in self.routers:
            self.hosts[0].cmdPrint(
                '../bin/forwardingtableclient --server {}:6474'.format(h.IP()))

    def edgeRouterCmd(
            self,
            ip,
            num_threads=50,
            num_pending_clients=2,
            min_forward_time=0,
            max_forward_time=0):
        """
        Return a string containing the edge router command to run based on the experiment type

        Parameters:
        ----------
        ip:    string
               The IP address of the edge router
        num_threads: int
               The number of threads to be spawn
        num_pending_clients: int
               The maximum number of clients that can be waiting for a response
        min_forward_time: float
               The minimum time artificially added to the lambd processing, in fractional seconds
        max_forward_time: float
               The maximum time artificially added to the lambd processing, in fractional seconds

        Returns:
        ----------
        str
               String representing the command to be executed
        """

        ret = ('../bin/edgerouter '
               '--server-endpoint {}:6473 '
               '--configuration-endpoint {}:6474 '
               '--num-threads {} '
               '{}').format(ip, ip, num_threads, self.controller_conf)

        # lambda processor server configuration
        ret += (' --router-conf max-pending-clients={},'
                'min-forward-time={},'
                'max-forward-time={}').format(
                    num_pending_clients,
                    min_forward_time,
                    max_forward_time)

        # forwarding table configuration
        if 'li' in self.confopts['balancing']:
            ret += ' --table-conf type=least-impedance'
        elif 'rp' in self.confopts['balancing']:
            ret += ' --table-conf type=random'
        elif 'rr' in self.confopts['balancing']:
            ret += ' --table-conf type=round-robin'
        else:
            raise Exception('Unknown edge forwarding table configuration: ' +
                            self.confopts['balancing'])

        # weight upddate configuration
        if 'periodic' in self.confopts['balancing']:
            ret += ' --optimizer-conf type=trivial,period=5,stat=min'
        elif 'async' in self.confopts['balancing']:
            ret += ' --optimizer-conf type=async,alpha=0.95'
        elif 'none' in self.confopts['balancing']:
            ret += ' --optimizer-conf type=none'
        else:
            raise Exception(
                'Unknown weight updater configuration: ' + self.confopts['balancing'])

        return ret

    def edgeDispatcherCmd(
            self,
            ip,
            conf,
            num_threads=50,
            num_pending_clients=2,
            min_forward_time=0,
            max_forward_time=0):
        """
        Return a string containing the edge dispatcher command to run based on the experiment type

        Parameters:
        ----------
        ip:    string
               The IP address of the edge router
        conf:  string
               The ptime estimator configuration string
        num_threads: int
               The number of threads to be spawn
        num_pending_clients: int
               The maximum number of clients that can be waiting for a response
        min_forward_time: float
               The minimum time artificially added to the lambd processing, in fractional seconds
        max_forward_time: float
               The maximum time artificially added to the lambd processing, in fractional seconds

        Returns:
        ----------
        str
               String representing the command to be executed
        """

        ret = ('../bin/edgedispatcher '
               '--server-endpoint {}:6473 '
               '--configuration-endpoint {}:6474 '
               '--num-threads {} '
               '--router-conf max-pending-clients={},'
               'min-forward-time={},'
               'max-forward-time={} '
               '--ptimeest-conf {} '
               '{}').format(
                   ip,
                   ip,
                   num_threads,
                   num_pending_clients,
                   min_forward_time,
                   max_forward_time,
                   conf,
                   self.controller_conf)

        return ret

    def saveJsonConf(self):
        "Save the current experiment configuration to a JSON file to be used by other tools"

        for r in self.routers:
            if 'routers' not in self.jconf:
                self.jconf['routers'] = []
            self.jconf['routers'].append(r.IP())

        for endpoints in self.computers.values():
            for e in endpoints:
                if 'computers' not in self.jconf:
                    self.jconf['computers'] = []
                self.jconf['computers'].append(e[0])

        for s in self.switches:
            if 'switches' not in self.jconf:
                self.jconf['switches'] = []
            self.jconf['switches'].append(s.name)

        print "Saving JSON configuration"
        with open('conf.json', 'w') as jdump:
            json.dump(self.jconf, jdump)

    def saveGraphviz(self, dotfilename):
        "Save the current topology to a graph using Graphviz"

        with open('{}.dot'.format(dotfilename), 'w') as dotfile:
            dotfile.write('graph G {\n')
            dotfile.write('overlap=scale;\n')
            dotfile.write('node [shape=ellipse];\n')
            for l in self.net.links:
                dotfile.write(
                    '{} -- {};\n'.format(l.intf1.node.name, l.intf2.node.name))
            for h in self.hosts:
                dotfile.write('{} [shape=rectangle]\n'.format(h.name))
            for h in self.nathosts:
                dotfile.write('{} [shape=triangle]\n'.format(h.name))
            dotfile.write('}\n')

        subprocess.Popen(
            ['neato', '-Tpng',
             '-o{}.png'.format(dotfilename),
             '{}.dot'.format(dotfilename)])
        subprocess.Popen(
            ['neato', '-Tsvg',
             '-o{}.svg'.format(dotfilename),
             '{}.dot'.format(dotfilename)])

    def runExperiment(self, topo):
        "Run the experiment"

        hostcls = Host \
            if ('cpu_limit' not in self.confopts or self.confopts['cpu_limit'] < 0) \
            else CPULimitedHost
        self.net = Mininet(
            topo,
            link=TCLink,
            host=hostcls,
            switch=OVSKernelSwitch,
            ipBase='10.1.0.0/16',
            controller=RemoteController('c0', ip='127.0.0.1', port=6633))
        #self.net = Mininet(topo, link=TCLink, switch=OVSKernelSwitch)

        if 'nat' in self.confopts and self.confopts['nat']:
            self.net.addNAT().configDefault()

        self.net.start()
        # self.net.waitConnected()

        if self.confopts['verbosity'] >= 1:
            print "Dumping host connections"
            dumpNodeConnections(self.net.hosts)

        print "Analyzing topology"
        for i in self.net:
            break
        for i in self.net:
            if isinstance(self.net.get(i), Host):
                self.hosts.append(self.net.get(i))
            elif isinstance(self.net.get(i), NAT):
                self.hosts.append(self.net.get(i))
                self.nathosts.append(self.net.get(i))
            elif isinstance(self.net.get(i), Switch):
                self.switches.append(self.net.get(i))
            elif isinstance(self.net.get(i), Controller):
                if 'c0' in locals():
                    raise Exception(
                        "There cannot be two or more controller nodes")
                c0 = self.net.get(i)
            else:
                raise Exception(
                    "Unknown node type: {}.".format(self.net.get(i)))

        if len(self.hosts) == 0:
            raise Exception("There must be at least one host")
        if len(self.switches) == 0:
            raise Exception("There must be at least one switch")
        if 'c0' not in locals():
            raise Exception("There must be a controller node")

        if 'save_topo' in self.confopts and len(self.confopts['save_topo']) > 0:
            self.saveGraphviz(self.confopts['save_topo'])

        if self.confopts['cli'] == 0:
            CLI(self.net)
            return

        print "Creating flows"
        for ln in self.net.links:
            node1, if1 = ln.intf1.name.split('-')
            node2, if2 = ln.intf2.name.split('-')
            if node1 not in self.graph:
                self.graph[node1] = {}
            self.graph[node1][node2] = if1[3:]
            if node2 not in self.graph:
                self.graph[node2] = {}
            self.graph[node2][node1] = if2[3:]

        with open('dist.txt', 'w') as distfile:
            for i in self.hosts:
                distfile.write('{}'.format(i.IP()))
                distances = dijkstra.spt(self.graph, i.name)[1]
                for j in self.hosts:
                    distfile.write(' {}'.format(distances[j.name]))
                distfile.write('\n')

        outport_by_dst = dict()
        for h in self.hosts:
            if self.confopts['verbosity'] >= 1:
                print "{}: {}".format(h.IP(), dijkstra.spt(self.graph, h.name))
            for node, prev in dijkstra.spt(self.graph, h.name)[0].items():
                sw = self.net.get(node)
                if not isinstance(sw, Switch):
                    continue
                out_port = self.graph[sw.name][prev]
                for in_port in self.graph[sw.name].values():
                    if in_port == out_port:
                        continue
                    c0.cmd(("ovs-ofctl "
                            "add-flow unix:/var/run/openvswitch/{}.mgmt "
                            "in_port={},eth_type=0x800,ip_dst={},actions=output:{}").format(
                                sw.name,
                                in_port,
                                h.IP(),
                                out_port))
                    if h.IP() == self.nat_ip:
                        c0.cmd(("ovs-ofctl "
                                "add-flow unix:/var/run/openvswitch/{}.mgmt "
                                "in_port={},eth_type=0x800,nw_dst={},actions=output:{}").format(
                                    sw.name,
                                    in_port,
                                    self.nat_network,
                                    out_port))
                    if sw.name not in outport_by_dst:
                        outport_by_dst[sw.name] = dict()
                    outport_by_dst[sw.name][h.IP()] = out_port

        print "Computing the end-to-end paths affected by any link failure"
        self.jconf['affected_paths'] = dict()
        for i in self.hosts:
            spt = dijkstra.spt(self.graph, i.name)[0]
            for j in self.hosts:
                if i == j:
                    continue
                intermediates = dijkstra.traversing(spt, j.name, i.name)
                ports = []
                for k in intermediates:
                    ports.append(outport_by_dst[k][i.IP()])
                for pair in zip(intermediates, ports):
                    key = '{},{}'.format(pair[0], pair[1])
                    if key not in self.jconf['affected_paths']:
                        self.jconf['affected_paths'][key] = []
                    self.jconf['affected_paths'][key].append((j.IP(), i.IP()))
                if self.confopts['verbosity'] >= 1:
                    print '{} -> {} = {}'.format(j.IP(),
                                                 i.IP(), zip(intermediates, ports))

        print "Dumping flow static tables"
        for sw in self.switches:
            c0.cmdPrint(
                "ovs-ofctl dump-flows unix:/var/run/openvswitch/{}.mgmt".format(sw.name))

        print "Filling static ARP tables"
        for src in self.hosts:
            for tgt in self.hosts:
                if src != tgt:
                    src.setARP(tgt.IP(), tgt.MAC())

        #print "Waiting for controller"
        # while (self.net.pingPair() != 0):
        #    pass

        if self.nathosts:
            print "NAT hosts\n{}".format(self.nathosts)

        print "Testing network connectivity"
        if 'pingtest' in self.confopts and self.confopts['pingtest'] == 'all':
            self.net.pingAll()
        elif 'pingtest' in self.confopts and self.confopts['pingtest'] == 'none':
            print "Skipped"
        elif 'pingtest' not in self.confopts and self.confopts['pingtest'] == "single":
            for h in self.hosts:
                if h == self.hosts[0] or h in self.nathosts:
                    continue
                if self.net.ping([self.hosts[0], h]) != 0:
                    raise Exception(
                        "Ping from {} to {} failed, bailing out".format(self.hosts[0].name, h.name))
        else:
            raise RuntimeError("Invalid 'pingtest' option: {}".format(self.confopts['pingtest']))

        # create the controller, if needed
        self.createEdgeController()

        if self.confopts['cli'] == 1:
            CLI(self.net)
            return

        # create edge routers and computers
        self.setupEdgeResources()

        # create the collectors of utilization values from computers
        self.createUtilizationCollectors()

        # manually install forwarding tables into edge routers, if necessary
        self.installForwardingTables()

        # wait until all the edge routers respond to the execution of clambda0
        self.waitForEdgeRoutersReady()

        print "Starting OF switches statistics collection"
        self.cmd.run(self.hosts[0],
                     (
                         '../../scripts/ovs-stat.py '
                         '--switches {} '
                         '--interval 5 '
                         '--metrics bytes '
                         '--total '
                         '--rate '
                         '--output results/ovsstat.{}'
        ).format(
            ','.join(
                sorted([sw.name for sw in self.switches])),
            self.experiment_id),
            is_background=True)

        self.saveJsonConf()

        if 'tclink' in self.confopts and self.confopts['tclink'] == 'wtc2':
            print "Starting OF switches monitor"
            self.hosts[0].cmd(
                '../../scripts/ovs-monitor.py '
                '--conf conf.json '
                '--interval 1 '
                '--threshold 1.5e6 '
                '--forwarding_table_client ../bin/forwardingtableclient '
                '--loglevel warning '
                '--dry_run '
                '>& monitor.log &')

        if self.confopts['cli'] == 2:
            CLI(self.net)
            return

        self.createTraffic()

        self.saveForwardingTableSize()

        if self.confopts['cli'] == 3:
            CLI(self.net)

    def createTraffic(self):
        """Does nothing in the base class"""

        pass

    def setupEdgeResources(self):
        """Does nothing in the base class"""

        pass

    def __del__(self):
        print "Cleaning up"

        if len(self.hosts) >= 1:
            self.hosts[0].cmd('killall ovs-stat.py')
            self.hosts[0].cmd('killall reset_tables.sh')
            self.hosts[0].cmd('killall edgecontroller')

        for h in self.hosts:
            h.cmd('killall edgecomputer')
            h.cmd('killall edgerouter')
            h.cmd('killall edgecomputerclient')

        self.net.stop()
