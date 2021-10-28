#!/bin/bash

function run {
  if [ $dryrun -ne 1 ] ; then
    mkdir log results 2> /dev/null
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

    mangle="e=$e.b=$b.s=$s.l=$l.i=$i"
    
    cmd="python $script_name --experiment $e --bw $b --states $s --length $l --inputsize $i --duration $duration --numclients 1 --seed $x"

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
  meandir=derived

  echo "not implemented"
  return

  mkdir -p $meandir 2> /dev/null

  oldpwd=$PWD
  cd results

  rm -f $oldpwd/$meandir/ 2> /dev/null

  for t in $links ; do
  for e in $experiments ; do

    mangle_out="t=$t.e=$e"
    echo $mangle_out

    for l in $losses ; do

      mangle_in="t=$t.l=$l.e=$e.1"


      ret=$($percentile --col 2 --mean --quantiles 0.90 0.95 0.99 < out.$mangle_in)
      echo "$l $(grep "+-" <<< $ret | cut -f 1 -d ' ') $(grep "+-" <<< $ret | cut -f 3 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-avg.dat
      echo "$l $(grep ^"0.9 " <<< $ret | cut -f 2 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-090.dat
      echo "$l $(grep ^"0.95 " <<< $ret | cut -f 2 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-095.dat
      echo "$l $(grep ^"0.99 " <<< $ret | cut -f 2 -d ' ')" >> $oldpwd/$meandir/out-$mangle_out-099.dat

      ret=$($percentile --col -1 --mean < ovsstat.$mangle_in)
      echo "$l $(grep "+-" <<< $ret | cut -f 1 -d ' ') $(grep "+-" <<< $ret | cut -f 3 -d ' ')" >> $oldpwd/$meandir/tpt-$mangle_out-avg.dat

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
percentile="percentile.py --warmup_col 1 --warmup 10 --duration $duration"
confidence="confidence.py --alpha 0.05"
script_name="simple_function_chain.py"

states="3 6"
lengths="3 6"
bw="1 2 5 10 20 50 100"
inputsizes="10000 100000"
experiments="embedded hopbyhop"
$1
