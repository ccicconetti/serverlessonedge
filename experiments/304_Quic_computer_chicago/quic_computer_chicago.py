#!/usr/bin/python
"""QUIC serverless access in a realistic IoT topology"""

import random
import time

import dijkstra
import experiment
import topo


class QuicComputerChicago(experiment.Experiment):
    """One edge dispatcher in the leftmost host, all other nodes are edge computers"""

    def __init__(self, **kwargs):
        experiment.Experiment.__init__(self, **kwargs)

        self.experiment_id = '.c={}.s={}.e={}.l={}.{}'.format(
            self.confopts['n_clients'],
            self.confopts['n_servers'],
            self.confopts['experiment'],
            self.confopts['lambda_protocol'],
            self.confopts['seed']
        )

    def setupEdgeResources(self):
        """Set up edge router and computers"""

        # random drop the servers
        self.servers = random.sample(self.hosts, self.confopts['n_servers'])

        # select the centralized edge dispatcher with experiment probe
        if self.confopts['experiment'] == 'distributed':
            for i in self.servers:
                self.routers.append(i)
        elif self.confopts['experiment'] == 'centralized':
            self.routers.append(random.choice(self.hosts))

        # select the right edge server implementation
        if self.confopts['lambda_protocol'] == 'grpc':
            router_server_conf = 'grpc,persistence=0.05 '
        elif self.confopts['lambda_protocol'] == 'quic':
            router_server_conf = 'quic,attempt-early-data=false '
        else:  # quic-0rtt
            router_server_conf = 'quic,attempt-early-data=true '

        # start the edge routers, if needed
        if self.routers:
            print "Starting edge routers"
        for h in self.routers:

            router_cmd = (
                '{} --server-conf type={}').format(
                self.edgeRouterCmd(
                    ip=h.IP(),
                    num_threads=50,
                    num_pending_clients=50),
                router_server_conf)

            self.cmd.run(
                h,
                router_cmd,
                is_background=True)

        time.sleep(1)

        print "Starting edge computers"
        lambda_name = 'clambda0'
        conf_file_name = 'computer0.json'
        self.makeComputerJson(
            conf_file_name,
            [lambda_name],
            speed=1e11,
            num_cores=2,
            op_coeff=1e4,
            op_offset=0,
        )
        for h in self.servers:

            computer_server_conf = router_server_conf if self.confopts[
                'lambda_protocol'] == 'grpc' else 'quic '  # attempt early data not needed for edgecomputer (only server side)
            computer_conf = (
                '../bin/edgecomputer '
                '--conf type=file,path={} '
                '--num-thread 10 '
                '--server-conf type={}'
                '{}').format(
                    conf_file_name,
                    computer_server_conf,
                    self.controller_conf)

            # assign the computer's end-points incrementally
            lambda_proc_endpoint = '{}:10000'.format(h.IP())
            util_collector_endpoint = '{}:20000'.format(h.IP())

            self.cmd.run(
                h,
                '{} --server-endpoint {} --utilization {}'.format(
                    computer_conf,
                    lambda_proc_endpoint,
                    util_collector_endpoint),
                is_background=True)
            if h not in self.computers:
                self.computers[h] = []
            self.computers[h].append(
                [lambda_proc_endpoint, util_collector_endpoint])

    def createTraffic(self):
        "Create lambda request clients"

        # compute the set of possible destinations from any host
        # we take all the destinations with lower distance that
        # in a set of at least dest_pool_size
        destinations = None
        dest_pool_size = None

        destinations = self.routers
        dest_pool_size = 1

        assert self.confopts['n_servers'] >= dest_pool_size
        assert destinations is not None and dest_pool_size is not None

        pool = {}
        for i in self.hosts:
            spt = dijkstra.spt(self.graph, i.name)[0]
            dist = []
            for j in destinations:
                if i == j:
                    dist.append((0, j.name))
                else:
                    dist.append((
                        len(dijkstra.traversing(spt, j.name, i.name)),
                        j.name))

            # sort by increasing distance
            dist.sort()
            pool[i] = []
            last_cost = None
            for cost, server in dist:
                if len(pool[i]) >= dest_pool_size and last_cost is not None and cost > last_cost:
                    break

                endpoint = "{}:{}".format(self.net.get(server).IP(), 6473)

                assert endpoint is not None
                pool[i].append(endpoint)
                last_cost = cost

        print "Pool of destinations from clients"
        for src, dst in pool.items():
            print "{} -> {}".format(
                src.name,
                ','.join([x for x in dst]))

        print "Starting traffic"

        if self.confopts['lambda_protocol'] == 'grpc':
            client_conf = 'grpc,persistence=0.1'
        elif self.confopts['lambda_protocol'] == 'quic':
            client_conf = 'quic,attempt-early-data=false'
        else:
            client_conf = 'quic,attempt-early-data=true'

        for counter in range(self.confopts['n_clients']):
            h = random.choice(self.hosts)

            self.cmd.run(
                h,
                ('../bin/edgeclient '
                 '--server-endpoint {} '
                 '--client-conf type={} '
                 '--lambda clambda0 '
                 '--num-threads 1 '
                 '--sizes 1000 '
                 '--inter-request-time 1 ' 
                 '--seed {} '
                 '--output-file {}').format(
                    ','.join(random.sample(pool[h], dest_pool_size)),
                    client_conf,
                    self.confopts['seed'] * 1000 + counter,
                    'results/out.{}.{}'.format(self.experiment_id, counter)),
                is_background=True)

        print "Waiting for the experiment to complete"
        time.sleep(self.confopts['duration'])
        for h in self.hosts:
            h.cmd('killall edgeclient')


if __name__ == '__main__':
    args = experiment.Experiment.makeArgumentParser(
        {
            'n_servers': [
                int,
                1,
                'number of servers'],
            'n_clients': [
                int,
                1,
                'number of clients'],
            'experiment': [
                str,
                'distributed',
                'experiment type, one of: distributed, centralized'
            ],
            'lambda_protocol': [
                str,
                'grpc',
                'one of: grpc, quic, quic-0rtt'
            ]
        },
        {
            'cpu_limit': -1.0,
            'balancing': 'rr-async',
            'nat': False,
            'edgetype': 'router',
            'tclink': 'wotc',
            'pingtest': 'none'
        })

    experiment = QuicComputerChicago(**vars(args))

    experiment.runExperiment(
        topo.BriteTopo(
            args.verbosity >= 1,
            'nodes.topo',
            dict(
                bw=10,
                delay='10ms',
                loss=0,
                max_queue_size=1000,
                use_htb=True)))
