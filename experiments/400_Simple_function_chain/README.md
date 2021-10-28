# Experiment 400

## Description

Nodes arranged in a chain with varying characteristics (bandwidth, latency, loss).

One client invoking functions periodically on an e-computer.

## Execution

### Prerequisites

1. compile ServerlessOnEdge executables (see [building instructions](../../docs/BUILDING.md)) in `release` mode
2. create symbolic links in `experiments/bin` by executing from the current scenario directory:

```
pushd ../bin && ./mklinks.sh release && popd
```

3. verify that 1. and 2. have been executed correctly by running `../bin/edgerouter`, which should launch an e-router (terminate with Ctrl+C)
4. add the `experiments/Common` directory to `PYTHONPATH` by executing from the current scenario directory:

```
pushd ../../ && source utils/environment && popd
```

5. verify that 4. has been executed correctly by running `python -c "import topo"`, which should terminate without exceptions

The steps 1-3 must be carried out only once or upon changing/updating the ServerlessOnEdge codebase.

The steps 4-5 must be re-executed at each new shell.

### Running the experiments

To run all the experiments execute:

```
sudo ./experiments.sh run
```

The execution will produce raw results in a directory called `results` and log files (one per experiment) in `log`.
Upon a new execution of the command above, a given experiment will be skipped if there is already its log file, thus if you want to interrupt the execution of a batch and experiments and resume later, you just have to make sure that the log files of the experiments to be skipped are present in `log` (the content does not matter, indeed files can be empty but they must be readable).

If you want to check the full list of the experiments that will be run, you can issue:

```
./experiments.sh enumerate
```

### Downloading the raw results without running the experiments

The raw results are also available for download:

```
wget http://turig.iit.cnr.it/~claudio/public/soe-results-400.tgz
```

### Post-processing

The raw results can be analyzed via:

```
./experiments.sh analyze
```

which produces synthetic results in the directory `derived`.

Note that executing the post-processing script requires steps 4-5 of the prerequisites above.

### Visualization

Some Gnuplot scripts for visualization are provided in the `graphs` directory.
