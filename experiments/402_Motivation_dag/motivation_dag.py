#!/usr/bin/env python2.7
"""Motivation dag experiment"""

import json
import os
import random
import time

from mininet.topo import Topo
import experiment
import topo


class MotivationTopo(Topo):
    """Edge-cloud topology. The edge network is made of many nodes, each
    consisting of a switch and a host. The cloud and the client each consist
    of a single node. All the nodes are connected to a central switch.
    """

    def __init__(self):
        """Constructor
        """

        # initialize topology
        Topo.__init__(self)

        # configuration
        num_edges = 4
        num_clients = 1
        bw_lat_edge = [1000, "1ms"]
        bw_lat_cloud = [10, "20ms"]

        # create the hosts
        #                       0 <=  cloud  <= 0
        #                       1 <= clients <= num_clients
        #           1+num_clients <=  edges  <= num_clients+num_edges
        # 1+num_clients+num_edges <= central <= 1+num_clients+num_edges
        central = 1 + num_clients+num_edges
        num_nodes = 2 + num_clients + num_edges
        hosts = []
        for i in range(num_nodes):
            hosts.append(self.addHost('h%s' % (i + 1)))

        # create the switches
        switches = [self.addSwitch('s%s' % (s + 1))
                    for s in range(num_nodes)]

        # add one host to each switch and connect the central switch to the other nodes
        for i in range(num_nodes):
            self.addLink(hosts[i], switches[i])
            if i == central:
                continue
            cur_bw = bw_lat_cloud[0] if i == 0 else bw_lat_edge[0]
            cur_lat = bw_lat_cloud[1] if i == 0 else bw_lat_edge[1]
            self.addLink(
                switches[i],
                switches[central],
                bw=cur_bw,
                delay=cur_lat,
                loss=0, max_queue_size=300, use_htb=True)


class MotivationDag(experiment.Experiment):
    """Single client with a DAG-based workflow. The e-computers are always
    on the edge nodes, while the status can be located either in the e-computer
    or in the cloud node."""

    def __init__(self, **kwargs):
        experiment.Experiment.__init__(self, **kwargs)

        self.experiment_id = "e={}.s={}.{}".format(
            self.confopts["experiment"],
            self.confopts["statesize"],
            self.confopts["seed"]
        )

        self.num_edges = 4
        self.num_clients = 1
        self.num_nodes = 2 + self.num_clients + self.num_edges

        self.cloud = 0
        self.client_min = 1
        self.edge_min = 1 + self.num_clients
        self.central = 1 + self.num_clients + self.num_edges

    def setupEdgeResources(self):
        """Set up edge router and computers"""

        print "Starting edge routers"
        self.routers = self.hosts
        for h in self.routers:
            self.cmd.run(
                h,
                self.edgeRouterCmd(
                    ip=h.IP(), num_threads=5, num_pending_clients=5
                ),
                is_background=True)
        time.sleep(1)

        print "Starting edge computers"
        stateful_lambdas = ["sync", "fe_audio", "fe_video", "decision"]
        assert(self.num_edges >= len(stateful_lambdas))
        for i in range(self.num_nodes):
            h = self.hosts[i]
            conf_file_name = "computer-{}.json".format(h.IP())

            cur_lambdas = []
            if i == 0:
                cur_lambdas += ["dummy"]
            elif self.client_min <= i < (self.client_min + self.num_clients):
                cur_lambdas += ["start"]
            elif self.edge_min <= i < (self.edge_min + self.num_edges):
                cur_lambdas += ["anon_audio", "anon_video"]
                if (i - self.edge_min) < len(stateful_lambdas):
                    cur_lambdas.append(stateful_lambdas[i - self.edge_min])
            else:
                continue

            self.makeComputerJson(
                conf_file_name,
                cur_lambdas,
                speed=float(1e12),
                num_cores=2,
                op_coeff=float(1e3),
                op_offset=0,
            )

            # assign the computer's end-points
            lambda_proc_endpoint = "{}:10000".format(h.IP())
            util_collector_endpoint = "{}:20000".format(h.IP())

            # run the edgecomputer
            self.cmd.run(
                h,
                (
                    "../bin/edgecomputer "
                    "--conf type=file,path={} "
                    "--num-thread 5 "
                    "--server-endpoint {} "
                    "--utilization-endpoint {} "
                    "--asynchronous "
                    "--companion-endpoint {}:6473 "
                    "--state-endpoint {}:30000 "
                    "{}"
                ).format(
                    conf_file_name,
                    lambda_proc_endpoint,
                    util_collector_endpoint,
                    h.IP(),
                    h.IP(),
                    self.controller_conf),
                is_background=True
            )

            self.computers[h] = [
                [lambda_proc_endpoint, util_collector_endpoint]]

    def createDagFile(self, filename="dag.json"):
        dag = dict()

        dag["successors"] = [[1, 2], [3, 4], [3, 5], [6], [6], [6]]
        dag["functionNames"] = ["start", "anon_audio", "anon_video",
                                "sync", "fe_audio", "fe_video", "decision"]
        dag["dependencies"] = {
            "state_fe_audio": ["fe_audio"],
            "state_fe_video": ["fe_video"],
            "state_sync": ["sync"],
            "state_decision": ["decision"],
        }
        dag["state-sizes"] = {
            "state_fe_audio": 100 * self.confopts["statesize"],
            "state_fe_video": 1000 * self.confopts["statesize"],
            "state_sync": 10 * self.confopts["statesize"],
            "state_decision": 10 * self.confopts["statesize"],
        }

        with open(filename, "w") as outfile:
            json.dump(dag, outfile)

    def createTraffic(self):
        """Create lambda request clients"""

        self.clients = [self.hosts[self.client_min + x]
                        for x in range(self.num_clients)]

        print "Client(s): {}".format(self.clients)

        print "Starting traffic"
        self.createDagFile("dag.json")
        assert(len(self.clients) == 1)
        h = self.clients[0]
        result_file = "results/out.{}".format(self.experiment_id)
        if os.path.isfile(result_file):
            os.remove(result_file)
        for invocation in range(self.confopts["duration"]):
            self.cmd.run(
                h,
                (
                    "../bin/edgeclient "
                    "--dag-conf type=file,filename=dag.json "
                    "--server-endpoint {}:6473 "
                    "--num-threads 1 "
                    "--inter-request-time 0 "
                    "--max-requests 1 "
                    "--sizes 1000000 "
                    "{} "
                    "{} "
                    "--seed {} "
                    "--append "
                    "--output-file {} "
                ).format(
                    h.IP(),
                    "--callback {}:6480".format(
                        h.IP()) if self.confopts["experiment"] in [
                            "edge", "cloud"] else "",
                    "--state-endpoint {}:6481".format(
                        h.IP()) if self.confopts["experiment"] == "edge" else "",
                    self.confopts["seed"],
                    result_file
                ),
                is_background=False,
            )


if __name__ == "__main__":
    args = experiment.Experiment.makeArgumentParser(
        {
            "experiment": [
                str,
                "client",
                "experiment type, one of: client, edge, cloud",
            ],
            "statesize": [int, 1000, "state size, in kB"],
        },
        {
            "cpu_limit": -1.0,
            "nat": False,
            "controller": "flat",
            "balancing": "rr-async",
            "edgetype": "router",
            "tclink": "wotc",
            "pingtest": "none",
        },
    )

    experiment = MotivationDag(**vars(args))

    assert experiment.confopts["experiment"] in [
        "client", "edge", "cloud"]
    assert experiment.confopts["statesize"] > 0

    experiment.runExperiment(MotivationTopo())
