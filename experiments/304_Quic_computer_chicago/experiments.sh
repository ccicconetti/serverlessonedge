#!/bin/bash

function run {
  if [ $dryrun -ne 1 ] ; then
    mkdir log results 2> /dev/null
  fi

  for x in $seed ; do
  for e in $experiments ; do
  for l in $lambdaProtocol ; do

    mangle="l=$l.e=$e.$x"

    cmd="python quic_computer_chicago.py --experiment $e --n_clients 40 --n_servers 6 --seed $x --lambda_protocol $l --duration $duration"

    echo $cmd
    if [ $dryrun -ne 1 ] ; then
      if [ ! -r log/$mangle.log ] ; then
        $cmd >& log/$mangle.log
      else
        echo "skipped"
      fi
    fi

  done
  done
  done
}

function enumerate {
  dryrun=1
  run
}

function print_help {
  cat << EOF
Syntax: `basename $0` <run|enumerate>
Accepted options:
  -h: print this help
EOF
  exit 0
}

OPTIONERROR=0
OPTIND=1
while getopts "h?" opt; do
  case "$opt" in
    h|\?)
      print_help
      ;;
  esac
done

shift $((OPTIND-1)) # Shift off the options and optional --.

if [ $OPTIONERROR -gt 0 ]; then
  exit -1
fi

if [ $# -ne 1 ] ; then
  print_help
fi

dryrun=0 # set automatically if command 'enumerate' is given

#
# configuration starts here
#
seed="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30"
duration=60
percentile="percentile.py --warmup_col 1 --warmup 10 --duration $duration"
confidence="confidence.py --alpha 0.05"

experiments="distributed centralized"
lambdaProtocol="grpc quic quic0rtt"
$1
