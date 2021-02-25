#!/usr/bin/python
"""Edge router forwarding table updater according to congestion
   on intermediate paths between edge routers and edge computers."""

import logging
import subprocess

class ForwardingTableUpdater(object):
    """Update the forwarding tables on edge routers based on congestion indication"""

    def __init__(self, computers, routers, paths, client, dry_run):
        self.computers = computers
        self.routers = routers
        self.paths = paths
        self.client = client
        self.status = self.good_status()
        self.dry_run = dry_run

    @staticmethod
    def mangle(router, computer):
        """Mangle the router and computer addresses into a single string"""
        return '{},{}'.format(router,computer)

    def good_status(self):
        """Return a status with all enabled pairs router-computer"""

        ret = dict()
        for router in self.routers:
            for computer in self.computers:
                ret[self.mangle(router,computer)] = True
        return ret

    def disable(self, pair):
        """Disable the given edge computer destination on the given edge router"""

        (router, computer) = pair.split(',')

        logging.warning('disabling destination %s on router %s', computer, router)

        if self.dry_run:
            return

        p = subprocess.Popen(
            [self.client,
             '--action',
             'remove',
             '--server-endpoint',
             '{}:6474'.format(router),
             '--destination',
             '{}:10000'.format(computer)],
            stdout=subprocess.PIPE)

        for line in p.stdout:
            if 'OK' in line:
                return

        logging.critical('invalid response from %s', router)

    def enable(self, pair):
        """Enable the given edge computer destination on the given edge router"""

        (router, computer) = pair.split(',')

        logging.warning('enabling destination %s on router %s', computer, router)

        if self.dry_run:
            return

        p = subprocess.Popen(
            [self.client,
             '--action',
             'change',
             '--server-endpoint',
             '{}:6474'.format(router),
             '--destination',
             '{}:10000'.format(computer)],
            stdout=subprocess.PIPE)

        for line in p.stdout:
            if 'OK' in line:
                return

        logging.critical('invalid response from %s', router)

    def update(self, links):
        """Perform actions based on the given list of congested links"""

        new_status = self.good_status()
        for link in links:
            for path in self.paths[link]:
                for router in self.routers:
                    for computer in self.computers:
                        if router in path and computer in path:
                            new_status[self.mangle(router,computer)] = False

        # disable all destinations that are affected by a congested link
        # that were not already disabled
        for key,value in new_status.items():
            if value is False and self.status[key] is True:
                self.disable(key)

        # restore all destinations previously disabled but that are not
        # affected by a congested link anymore
        for key,value in self.status.items():
            if value is False and new_status[key] is True:
                self.enable(key)

        # the current status becomes the next status
        self.status = new_status
