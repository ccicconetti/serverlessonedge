# Simulation 020

Simulations of execution of apps alternating between stateless (lambda) and stateful (mu) behavior.

# Dependencies

## lambdamusim

Check-out v1.4.0:

```
git checkout v1.4.0
```

See build instructions in the main repository `README.md`.

At the end of the procedure copy or symlink the `lambdamusim` executable to the current directory.

## ether

A sample topology is included in the `network` directory, which can be regenerated as follows (requires `virtualenv` and `python` >= 3, the other dependencies will be downloaded automatically):

```
git clone https://github.com/ccicconetti/ether.git
cd ether
git checkout lambdamusim-1
./run.sh
```

# Execution

After installing all the depencies above you may run simulations with:

```
./run.sh
```

The execution will produce its output in the directory `data`, which will be created if not existing.

Special commands:

- with `DRY=1` you obtain the list of commands what will be run
- with `EXPLAIN=1` you get an explanation of the fields in the output file
- with `PRINT=1` you get a comma-separated value list of the fields in the output file

## Downloading the raw results without running the experiments

The raw results are also available for download:

```
./download-raw.sh
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

which will produce post-processed files into the directory `post`.

## Visualization

A selection of plots can be produced by entering the directory `graphs` and executing the corresponding scripts with [Gnuplot](http://www.gnuplot.info/).
