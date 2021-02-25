# StateSim

This directory contains a stand-alone simulator of the execution of stateful FaaS chains of function invocations in an edge network.

## Objectives

The simulators takes as input:

- the topology of the edge network, in the output format of a [fork](https://github.com/ccicconetti/ether) of [_ether: Edge Topology Synthesizer_](https://github.com/edgerun/ether)
- the workload of the jobs to be executed, taken from the tasks produced by [_Spår: Cluster Trace Generator_](https://github.com/All-less/trace-generator)

The workload from Spår is actually transformed before being used in StateSim, because the former produces jobs made of stateless tasks in a Directed Acyclic Graph (DAG) while in StateSim we need a chain of stateful tasks.

After allocating the workload to the edge nodes with computing capabilities in the network in a greedy Shortest Processing Time First manner, StateSim estimates the execution time, network delay, and traffic volume according to one of different policies:

- PureFaaS: every function invocation is done by the client
- StatePropagate: any function calls the next one in the chain, the state is propagated from the client along the entire chain
- StateLocal: functions are chained, the state is retrieved by a function as needed from its previous location

## Usage

After compiling the codebase, you will find the executable in `StateSim/statesim` from the build directory. The command-line arguments can be retrieved with `StateSim/statesim --help`, which gives:

```
Allowed options:
  -h [ --help ]               produce help message
  --nodes-file arg (=nodes)   File containing the specifications of nodes.
  --links-file arg (=links)   File containing the specifications of links.
  --edges-file arg (=edges)   File containing the specifications of edges.
  --tasks-file arg (=tasks)   File containing the specifications of tasks.
  --outdir arg (=data)        The directory where to save the results.
  --num-functions arg (=5)    The number of lambda functions.
  --num-jobs arg (=10)        The number of jobs per replication.
  --seed-starting arg (=1)    The starting seed.
  --num-replications arg (=1) The number of replications.
  --ops-factor arg (=1000)    The multiplier of the number of operations of
                              loaded tasks.
  --mem-factor arg (=1000)    The multiplier of the memory/state size of loaded
                              tasks.
  --num-threads arg (=64)     The number of threads to spawn.
```

## Examples

Please have a look to the [simulations bundled in this repository](../simulations/), which also include pre-compiled network topologies and instructions on how to create an input workload.
