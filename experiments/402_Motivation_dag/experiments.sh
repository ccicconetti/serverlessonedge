#!/bin/bash

function run {
  if [ $dryrun -ne 1 ] ; then
    mkdir log results 2> /dev/null
    mn -c
  fi

  for x in $seed ; do
  for s in $statesizes ; do
  for i in $inputsizes ; do
  for e in $experiments ; do

    if [[ $s -eq $l ]] ; then
      continue
    fi

    mangle="e=$e.s=$s.i=$i.$x"
    
    cmd="python $script_name --experiment $e --inputsize $i --statesize $s --duration $duration --seed $x"

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

  for i in $inputsizes ; do
  for e in $experiments ; do

    mangle_out="e=$e.i=$i"
    echo $mangle_out

    for s in $statesizes ; do

      mangle_in="e=$e.s=$s.i=$i"

      ret=$(tail -n 95 out.$mangle_in.* | $percentile --col 2 --mean)
      echo "$s $(grep "+-" <<< $ret | cut -f 1 -d ' ') $(grep "+-" <<< $ret | cut -f 3 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-avg.dat

      ret=$(cat ovsstat.$mangle_in.* | $percentile --col -1 --mean --count)
      tot=$(grep "+-" <<< $ret | cut -f 1 -d ' ')
      num=$(grep "records" <<< $ret | cut -f 1 -d ' ')
      echo "$s $(echo "$tot * $num" | bc)" >> $oldpwd/$meandir/tpt-$mangle_out-avg.dat

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
duration=100
percentile="percentile.py --warmup_col -1"
confidence="confidence.py --alpha 0.05"
script_name="motivation_dag.py"

statesizes="1 2 5 10 20 50 100 200 500 1000"
inputsizes="2000"
experiments="client edge cloud"
$1
