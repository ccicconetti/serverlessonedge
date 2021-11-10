#!/bin/bash

function run {
  if [ $dryrun -ne 1 ] ; then
    mkdir log results 2> /dev/null
    mn -c
  fi

  for s in $states ; do
  for l in $lengths ; do
  for b in $bw ; do
  for i in $inputsizes ; do
  for e in $experiments ; do
  for x in $seed ; do

    if [[ $s -eq $l ]] ; then
      continue
    fi

    mangle="e=$e.b=$b.s=$s.l=$l.i=$i.$x"
    
    cmd="python $script_name --experiment $e --bw $b --states $s --length $l --inputsize $i --duration $duration --seed $x"

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
  done 
  done
  done
}

function analyze {
  percentile.py -h >& /dev/null
  if [ $? -ne 0 ] ; then
    echo "Cannot find 'percentile.py', did you load 'utils/environment'?"
    exit 1
  fi

  meandir=derived

  mkdir -p $meandir 2> /dev/null

  oldpwd=$PWD
  cd results

  rm -f $oldpwd/$meandir/ 2> /dev/null

  for s in $states ; do
  for l in $lengths ; do
  for i in $inputsizes ; do
  for e in $experiments ; do

    if [[ $s -eq $l ]] ; then
      continue
    fi

    mangle_out="e=$e.s=$s.l=$l.i=$i"
    echo $mangle_out

    for b in $bw ; do

      mangle_in="e=$e.b=$b.s=$s.l=$l.i=$i"

      ret=$(cat out.$mangle_in.* | $percentile --col 2 --mean --quantiles 0.90 0.95 0.99)
      echo "$b $(grep "+-" <<< $ret | cut -f 1 -d ' ') $(grep "+-" <<< $ret | cut -f 3 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-avg.dat
      echo "$b $(grep ^"0.9 " <<< $ret | cut -f 2 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-090.dat
      echo "$b $(grep ^"0.95 " <<< $ret | cut -f 2 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-095.dat

      ret=$(cat ovsstat.$mangle_in.* | $percentile --col -1 --mean)
      echo "$b $(grep "+-" <<< $ret | cut -f 1 -d ' ') $(grep "+-" <<< $ret | cut -f 3 -d ' ')" >> $oldpwd/$meandir/tpt-$mangle_out-avg.dat

    done

  done
  done
  done
  done

  cd $oldpwd
}

function enumerate {
  dryrun=1
  run
}

function print_help {
  cat << EOF
Syntax: `basename $0` <run|analyze|enumerate>
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
seed="1"
duration=120
duration=10
percentile="percentile.py --warmup_col 1 --warmup 0 --duration $duration"
confidence="confidence.py --alpha 0.05"
script_name="simple_function_dag.py"

states="6"
lengths="7"
bw="1.0 2.0 5.0 10.0 20.0 50.0 100.0"
bw="1.0 100.0"
inputsizes="10000 100000"
experiments="embedded hopbyhop remotestate"
$1
