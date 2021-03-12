# Simulation 002

Simulations of execution of stateless vs. stateful chains of functions with variable:

- topology: IIoT vs. urban sensing
- load, in number of jobs
- allocation policies: processing only vs. processing+network
- execution policies: all unchained versions currently implemented

# Dependencies

## statesim

Check-out v1.1.1:

```
git checkout v1.1.1
```

See build instructions in the main repository `README.md`.

At the and of the procedure copy or symlink the `statesim` executable to the current directory.

## spar

The source code of `spar` is available at [this repository](https://github.com/All-less/trace-generator), but you can install it in a virtual environment with [pip](https://pypi.org/project/pip/). Assuming that you have pip installed and Python 3.x, just issue:

```
virtualenv -p /usr/bin/python3 spar
source  spar/bin/activate
pip install spar
```

where you have to change `/usr/bin/python3` to the path of _your_ Python 3.x executable.

Assuming that there is a Python virtual environment called `spar` in the simulation directory, run the following script to generate a 1-hour long trace of tasks:

```
./create_tasks.sh
```

## ether

Two sample topologies for an IIoT and urban sensing scenario are included in the `network` directory.

To generate further topologies, download this forked repo, install dependencies, and run the examples in there:

```
git clone https://github.com/ccicconetti/ether.git
```

Note that the only change of the forker repo vs. the original ether codebase is the output format.

# Execution

First check that you have all dependencies installed. This includes:

- a symlink `statesim` in the current directory
- IIoT/urban sensing topologies in the `network` directory
- a virtual environment `spar` in the current directory from which `spar` can be executed (or a the spar-generated workload in `tasks/batch_task.csv`)

Then run the following commands to execute the full set of simulations:

```
NTHREADS=5 ./run.sh
```

If you want to change the number of concurrent threads used, you can specify a different number for `NTHREADS`.

The execution will produce its output in the directory `data`, which will be created by `statesim` if not existing.

If you want to obtain the list of the commands that will be run, without actually running them, prepend `DRY=1` to the command above.

## Downloading the raw results without running the experiments

The raw results are also available for download:

```
wget http://turig.iit.cnr.it/~claudio/public/soe-sim-results-002.tgz
```

## Post-processing

First make sure the scripts in `scripts/` are in the system `$PATH`, e.g., go to the root of the repository and run:

```
source utils/environment
```

You will need Python2.7 to execute the scripts required.

After having run all the simulations and set up your environment, you can post-process the simulation output by running:

```
./analyze.sh
```

which will produce post-processed files into the directory `results`. Note post-processing can take a long time to execute completely (hours) on the full simulation output.

Every file produced has four columns:

- column 1: the value of the x-axis
- column 2: the average of the values on the y-axis
- column 3: the bottom of the 95% conf interval of the values on the y-axis
- column 4: the top of the 95% conf interval of the values on the y-axis

The meaning of the x- and y-axis values depend on the file and can be inferred from its name.

## Visualization

A selection of plots can be produced by entering the directory `graphs` and executing the corresponding scripts with [Gnuplot](http://www.gnuplot.info/).
