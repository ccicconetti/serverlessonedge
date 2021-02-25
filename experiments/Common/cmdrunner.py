"""Run commands on mininet hosts"""

import threading

class CmdRunner(object):
    "Run commands on mininet hosts"

    def __init__(self, is_verbose):
        self.is_verbose = is_verbose
        self.counter = 0
        self.lock = threading.Lock()

    def run(self, host, cmd, is_background):
        "Run a command on a host"

        with self.lock:
            background = ''
            if is_background:
                background = ' & '
            log_head = ''
            log_tail = ''
            if self.is_verbose:
                log_head = 'GLOG_v=2 '
                log_tail = ' >& log.{}'.format(self.counter)
            else:
                log_tail = ' >& /dev/null'

            self.counter = self.counter + 1
            if is_background:
                host.cmdPrint(log_head + cmd + log_tail + background)
                return [host, int( host.cmd('echo $!') )]

        # release lock if we are not going in background
        host.cmdPrint(log_head + cmd + log_tail + background)
        return [None, None]

    def verbose(self, flag):
        "Change the verbosity level at run-time"

        with self.lock:
            self.is_verbose = flag
