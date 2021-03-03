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

function analyze {
  meandir=derived/mean

  mkdir -p $meandir 2> /dev/null

  oldpwd=$PWD
  cd results

  metrics="out-mean out-090 out-095 util-mean tpt-mean"

  for s in $numservers ; do
  for e in $experiments ; do

    mangle_out="s=$s.e=$e"

    echo $mangle_out

    rm $oldpwd/$meandir/*.tmp-* 2> /dev/null

    for c in $numclients ; do

      mangle_in="c=$c.$mangle_out"

      for x in $seed ; do
        tmp_out=tmp-$x

        rm tmp 2> /dev/null
        for infile in out.*.*.$x.$mangle_in.* ; do
          cat $infile | $percentile --col 2 --mean --quantiles 0.90 0.95  >> tmp
        done

        res=$(grep '+-' tmp | percentile.py --col 1 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-mean.$tmp_out
        res=$(grep ^'0.9 ' tmp | percentile.py --col 2 --quantile 0.90 | cut -f 2 -d ' ')
        #res=$(grep ^'0.9 ' tmp | percentile.py --col 2 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-090.$tmp_out
        res=$(grep ^'0.95 ' tmp | percentile.py --col 2 --quantile 0.95 | cut -f 2 -d ' ')
        #res=$(grep ^'0.95 ' tmp | percentile.py --col 2 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-095.$tmp_out

        cat util.*.*.$x.$mangle_in.* | $percentile --col 3 --mean > tmp

        res=$(grep +- tmp | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/util-mean.$tmp_out

        cat ovsstat.*.*.$x.$mangle_in | $percentile --col -1 --mean > tmp

        res=$(grep +- tmp | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/tpt-mean.$tmp_out

      done
    done

    for metric in $metrics ; do
      $confidence --xcol 1 --ycol 2 $oldpwd/$meandir/$metric.tmp-* > $oldpwd/$meandir/$metric.$mangle_out
    done

  done
  done

  rm $oldpwd/$meandir/*.tmp-* 2> /dev/null

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
seed="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30"
duration=60
percentile="percentile.py --warmup_col 1 --warmup 10 --duration $duration"
confidence="confidence.py --alpha 0.05"

experiments="distributed centralized"
lambdaProtocol="grpc quic quic0rtt"
$1
