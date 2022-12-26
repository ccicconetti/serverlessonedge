#!/usr/bin/env python2.7
"""Mvfst motivation experiment"""

import json
import os
import random
import time

from mininet.topo import Topo
import experiment
import topo


class MvfstMotivation(experiment.Experiment):
    """Single client connected to a single server via a link with variable characteristics."""

    def __init__(self, **kwargs):
        experiment.Experiment.__init__(self, **kwargs)

        self.experiment_id = "c={}.b={}.d={}.l={}".format(
            self.confopts["concurrency"],
            self.confopts["bw"],
            self.confopts["delay"],
            self.confopts["loss"]
        )
        

    def setupEdgeResources(self):
        """Set up edge router and computers"""

        print "Starting edge computer"

        conf_file_name = "computer.json"
        self.lambda_name = "lambda"

        self.makeComputerJson(
            conf_file_name,
            [self.lambda_name],
            speed=float(1e9),
            num_cores=self.confopts["concurrency"],
            op_coeff=float(1e4),
            op_offset=0,
        )

        h = self.hosts[1]

        # assign the computer's end-points
        self.lambda_proc_endpoint = "{}:10000".format(h.IP())
        util_collector_endpoint = "{}:20000".format(h.IP())

        # run the edgecomputer
        self.cmd.run(
            h,
            (
                "../bin/edgecomputer "
                "--conf type=file,path={} "
                "--num-thread {} "
                "--server-endpoint {} "
                "--utilization-endpoint {} "
                "--secure "
                "{}"
            ).format(
                conf_file_name,
                self.confopts["concurrency"],
                self.lambda_proc_endpoint,
                util_collector_endpoint,
                self.controller_conf),
            is_background=True
        )

        self.computers[h] = [
            [self.lambda_proc_endpoint, util_collector_endpoint]]


    def createTraffic(self):
        """Create lambda request clients"""

        print "Starting traffic"
        h = self.hosts[0]
        result_file = "results/out.{}".format(self.experiment_id)
        self.cmd.run(
            h,
            (
                "SECURE=1 "
                "SIZES=10,10000 "
                "NUMCALLS={} "
                "ENDPOINT={} "
                "CONCURRENCY={} "
                "LAMBDA={} "
                "OUTPUT={} "
                "./client.sh"
            ).format(
                self.confopts["numcalls"],
                self.lambda_proc_endpoint,
                self.confopts["concurrency"],
                self.lambda_name,
                result_file
            ),
            is_background=False,
        )


if __name__ == "__main__":
    args = experiment.Experiment.makeArgumentParser(
        {
            "concurrency": [int, 1, "number of concurrent operations"],
            "bw": [float, 100, "link bandwidth, in Mb/s"],
            "delay": [float, 1, "link delay, in ms"],
            "loss": [float, 0, "link packet loss probability, in %%"],
            "numcalls": [int, 10, "number of calls per thread"],
        },
        {
            "duration": 0,
            "dispatchertype": "rtt",
            "cpu_limit": -1.0,
            "nat": False,
            "controller": "flat",
            "balancing": "rr-async",
            "edgetype": "router",
            "tclink": "wotc",
            "pingtest": "none",
        },
    )

    experiment = MvfstMotivation(**vars(args))

    # check command-line args
    assert experiment.confopts["concurrency"] > 0
    assert experiment.confopts["bw"] > 0
    assert experiment.confopts["delay"] > 0
    assert experiment.confopts["loss"] >= 0
    assert experiment.confopts["numcalls"] > 0

    experiment.confopts["delay"] *= 1000 # from us to ms

    # check that the client executable exists
    if os.system("NUMCALLS=0 ./client.sh") != 0:
        raise RuntimeError("client executable invalid: check client.sh")

    # check that the SSL certificates are present, if not create them
    cert_files = [ 'ca.crt', 'server.crt', 'server.key', 'client.crt', 'client.key' ]
    cert_file_not_found = False
    for cf in cert_files:
        if not os.path.exists(cf):
            cert_file_not_found = True
            break
    if cert_file_not_found:
        if not os.path.exists('../../utils/create_cert.sh'):
            raise RuntimeError('cannot find the utility to generate new TLS certificates')

        if os.system("SERVER=localhost IP=10.1.0.2 ../../utils/create_cert.sh") != 0:
            raise RuntimeError('cannot generate new TLS certificates')

    # run experiment
    experiment.runExperiment(
        topo.LinearTopo(
            2,  # hosts
            dict(
                bw=experiment.confopts['bw'],
                delay=experiment.confopts['delay'],
                loss=experiment.confopts['loss'],
                max_queue_size=1000,
                use_htb=True),
            dict()))  # hostargs