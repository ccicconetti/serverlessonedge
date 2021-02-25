#!/usr/bin/python
"""Linear topology with one computer, one router and one client"""

import time

import experiment
import topo


class LinearQuic(experiment.Experiment):
    """One edge controller in the leftmost host followed by an edge computer, an edge router and an edge client in the rightmost one"""

    def __init__(self, **kwargs):
        experiment.Experiment.__init__(self, **kwargs)

        self.experiment_id = '{}.d.{}.s.{}.e.{}.l.{}.n.{}.{}'.format(
            self.confopts['bw'],
            self.confopts['delay'],
            self.confopts['size'],
            self.confopts['experiment'],
            self.confopts['loss'],
            self.confopts['numclients'],
            self.confopts['seed']
        )

    def setupEdgeResources(self):
        """Set up edge router and computers"""

        # one edgecomputer on hosts[1]
        self.servers = [self.hosts[1]]

        # one edegrouter on hosts[2]
        self.routers = [self.hosts[2]]

        print "Starting edge router"

        if self.confopts['experiment'] == 'grpc':
            router_server_conf = 'grpc,persistence=0.05 '
        elif self.confopts['experiment'] == 'quic':
            router_server_conf = 'quic,attempt-early-data=false '
        else:  # quic-0rtt
            router_server_conf = 'quic,attempt-early-data=true '

        router_cmd = (
            '{} --server-conf type={}').format(
            self.edgeRouterCmd(
                ip=self.hosts[2].IP(),
                num_threads=5,
                num_pending_clients=5),
            router_server_conf)

        self.cmd.run(
            self.hosts[2],
            router_cmd,
            is_background=True)
        time.sleep(1)

        print "Starting edge computers"

        lambda_name = 'clambda0'
        conf_file_name = 'computer.json'
        self.makeComputerJson(
            conf_file_name,
            [lambda_name],
            speed=float(1e12),
            num_cores=2,
            op_coeff=1e4,
            op_offset=0,
        )

        computer_server_conf = router_server_conf if self.confopts[
            'experiment'] == 'grpc' else 'quic '  # attempt early data not needed for edgecomputer (only server side)
        computer_conf = (
            '../bin/edgecomputer '
            '--conf type=file,path={} '
            '--num-thread 10 '
            '--server-conf type={}'
            '{}').format(
            conf_file_name,
            computer_server_conf,
            self.controller_conf)

        # assign the computer's end-points
        lambda_proc_endpoint = '{}:10000'.format(self.hosts[1].IP())
        util_collector_endpoint = '{}:20000'.format(self.hosts[1].IP())

        self.cmd.run(
            self.hosts[1],
            '{} --server-endpoint {} --utilization {}'.format(
                computer_conf,
                lambda_proc_endpoint,
                util_collector_endpoint),
            is_background=True)

        if self.hosts[1] not in self.computers:
            self.computers[self.hosts[1]] = []
        self.computers[self.hosts[1]].append(
            [lambda_proc_endpoint, util_collector_endpoint])

    def createTraffic(self):
        "Create lambda request clients"

        # one edgeclient on host[3]
        self.clients = [self.hosts[3]]

        print 'Client: {}'.format(self.clients)

        print "Starting traffic"
        pids = []
        counter = 0

        if(self.confopts['experiment'] == 'grpc'):
            client_conf = 'grpc,persistence=0.05'
        elif(self.confopts['experiment'] == 'quic'):
            client_conf = 'quic,attempt-early-data=false'
        else:
            client_conf = 'quic,attempt-early-data=true'

        server_endpoint = '{}:{}'.format(
            self.hosts[2].IP(), 6473)  # router

        pids.append(self.cmd.run(
            self.hosts[3],
            ('../bin/edgeclient '
             '--server-endpoint {} '
             '--client-conf type={} '
             '--num-threads 1 '
             '--inter-request-time 0.1 '
             '--lambda clambda0 '
             '--duration {} '
             '--sizes {} '
             '--seed {} '
             '--output-file {}').format(
                server_endpoint,
                client_conf,
                self.confopts['duration'],
                self.confopts['size'],
                1000 * self.confopts['seed'] + counter,
                'results/out.{}'.format(self.experiment_id)),
            is_background=True))
        counter += 1

        print "Waiting for the experiment to complete"

        # time.sleep(self.confopts['duration'])
        # print "Time is up!"
        # pids[0][0].cmd('kill', pids[0][1])

        pids[0][0].cmd('wait', pids[0][1])


if __name__ == '__main__':
    args = experiment.Experiment.makeArgumentParser(
        {
            'experiment': [
                str,
                'grpc',
                'experiment type, one of: grpc, quic, quic0rtt'],
            'bw': [
                float,
                100,
                'link bandwidth, one of: 0.01, 0.1, 1, 10, 100, 1000'
            ],
            'delay': [
                str,
                '100us',
                'link delays, one of: 1us, 100us, 10ms'
            ],
            'size': [
                int,
                1000,
                'lambda request input size, one of: 100, 1000, 10000'
            ],
            'loss': [
                float,
                0.01,
                'link loss probability'
            ],
            'numclients': [
                int,
                1,
                'number of client threads'
            ]
        },
        {
            'cpu_limit': -1.0,
            'nat': False,
            'controller': 'flat',
            'balancing': 'rr-async',
            'edgetype': 'router',
            'tclink': 'wotc'
        })

    experiment = LinearQuic(**vars(args))

    experiment.runExperiment(
        # topo.LinearLossTopo(
        topo.LinearTopo(
            4,  # hosts
            # linkargs
            dict(
                bw=experiment.confopts['bw'],
                delay=experiment.confopts['delay'],
                loss=experiment.confopts['loss'],
                max_queue_size=1000,
                use_htb=True),
            dict()))  # hostargs
