#!/usr/bin/env python2.7
"""Simple function chain experiment"""

import json
import os
import random
import time

import experiment
import topo


class SimpleFunctionChain(experiment.Experiment):
    """Nodes arranged in a circle, one is a client and all others are
       routers and computers."""

    def __init__(self, **kwargs):
        experiment.Experiment.__init__(self, **kwargs)

        self.experiment_id = "e={}.b={}.s={}.l={}.i={}.{}".format(
            self.confopts["experiment"],
            self.confopts["bw"],
            self.confopts["states"],
            self.confopts["length"],
            self.confopts["inputsize"],
            self.confopts["seed"]
        )

    def setupEdgeResources(self):
        """Set up edge router and computers"""

        # one edgecomputer on each host except 0
        self.servers = []
        for i in range(1, self.confopts["nodes"]):
            self.servers.append(self.hosts[i])

        # one edegrouter on each host
        self.routers = self.hosts

        print "Starting edge routers"

        for h in self.routers:
            self.cmd.run(
                h,
                self.edgeRouterCmd(
                    ip=h.IP(), num_threads=5, num_pending_clients=5
                ),
                is_background=True)
        time.sleep(1)

        print "Starting edge computers"

        for h in self.servers:
            conf_file_name = "computer-{}.json".format(h.IP())
            self.makeComputerJson(
                conf_file_name,
                [h.IP()],
                speed=float(1e12),
                num_cores=2,
                op_coeff=0.01 * float(1e12) / self.confopts["inputsize"],
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

    def createTraffic(self):
        """Create lambda request clients"""

        # one edgeclient on host[0]
        self.clients = [self.hosts[0]]

        print "Client: {}".format(self.clients)

        print "Starting traffic"
        pids = []
        counter = 0
        all_lambdas = [x.IP() for x in self.servers]

        result_file = "results/out.{}".format(self.experiment_id)
        if os.path.isfile(result_file):
            os.remove(result_file)

        while (counter < self.confopts["duration"]):
            for h in self.clients:
                chain = dict()
                chain["functions"] = []
                for f in range(self.confopts["states"]):
                    chain["functions"].append(random.choice(all_lambdas))
                chain["dependencies"] = dict()
                chain["state-sizes"] = dict()
                for s in range(self.confopts["length"]):
                    chain["dependencies"]["s{}".format(s)] = random.sample(
                        chain["functions"],
                        random.randrange(0, len(chain["functions"])))
                    chain["state-sizes"]["s{}".format(s)] = (1 + s) * 10000
                with open("chain-{}.json".format(h.IP()), "w") as outfile:
                    json.dump(chain, outfile)

                self.cmd.run(
                    h,
                    (
                        "../bin/edgeclient "
                        "--chain-conf type=file,filename=chain-{}.json "
                        "--server-endpoint {}:6473 "
                        "--num-threads 1 "
                        "--inter-request-time 0.1 "
                        "--max-requests 1 "
                        "--sizes {} "
                        "{} "
                        "{} "
                        "--seed {} "
                        "--output-file {} "
                        "--append"
                    ).format(
                        h.IP(),
                        h.IP(),
                        self.confopts["inputsize"],
                        "--callback {}:6480".format(
                            h.IP()) if self.confopts["experiment"] in [
                                "hopbyhop", "remotestate"] else "",
                        "--state-endpoint {}:6481".format(
                            h.IP()) if self.confopts["experiment"] == "remotestate" else "",
                        1000 * self.confopts["seed"] + counter,
                        result_file,
                    ),
                    is_background=False,
                )
                counter += 1

        # print "Waiting for the experiment to complete"
        # time.sleep(self.confopts["duration"])
        # for h in self.clients:
        #     h.cmd("killall edgeclient")


if __name__ == "__main__":
    args = experiment.Experiment.makeArgumentParser(
        {
            "experiment": [
                str,
                "embedded",
                "experiment type, one of: embedded, hopbyhop, remotestate",
            ],
            "bw": [float, 1, "bandwidth, in Mb/s"],
            "numclients": [int, 1, "number of client threads"],
            "nodes": [int, 6, "numer of nodes"],
            "states": [int, 4, "number of states"],
            "length": [int, 2, "chain length, in no. functions"],
            "inputsize": [int, 10000, "size of the input argument, in bytes"],
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

    experiment = SimpleFunctionChain(**vars(args))

    assert experiment.confopts["numclients"] >= 1
    assert experiment.confopts["length"] >= 1
    assert experiment.confopts["bw"] > 0
    assert experiment.confopts["inputsize"] > 0
    assert experiment.confopts["experiment"] in [
        "embedded", "hopbyhop", "remotestate"]

    experiment.runExperiment(
        topo.NestedCircularTopo(
            experiment.confopts["nodes"],
            0,
            dict(
                delay="1ms",
                bw=experiment.confopts["bw"],
                loss=0,
                max_queue_size=1000,
                use_htb=True,
            ),
            dict(),
        )
    )
