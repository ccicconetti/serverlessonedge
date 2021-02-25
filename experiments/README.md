# Emulation experiments with ServerlessOnEdge

This directory contains:

- `bin`: a directory that needs to contain the symbolic links to the ServerlessOnEdge executables (check the script `mklinks.sh` in there to create them automatically)
- `Common`: Python modules to be used to set up and execute the mininet eperiments
- scenario directories: for each directory there is an emulation scenario that uses mininet to run real-time experiments under controlled conditions

Every scenario comes with Bash scripts to run a batch of experiments with different parameters, depending on the scenario, and to post-process the experiment results, including Gnuplot scripts to generate a selection of graphs.
