#!/bin/bash

function run {
  if [ $dryrun -ne 1 ] ; then
    mkdir log results 2> /dev/null
  fi

  for n in $numclients ; do
  for l in $losses ; do
  for e in $experiments ; do
  for s in $seed ; do

    mangle="bw.1.d.10000us.s.1000.e.$e.n.$n.l.$l.$s"
    
    cmd="python $script_name --experiment $e --duration $duration --bw $bw --delay $delay --size 1000 --loss $l --numclients $n --seed $s"

    echo $cmd
    if [ $dryrun -ne 1 ] ; then
      $cmd >& log/$mangle.log
    fi

  done
  done 
  done
  done
}

function mean {
  meandir=derived/mean

  mkdir -p $meandir 2> /dev/null

  oldpwd=$PWD
  cd results

  metrics="out-mean out-090 out-095 out-099 util-mean util-fair tpt-mean loss-mean loss-fair"

  for e in $experiments ; do

    mangle_out="e=$e"

    rm $oldpwd/$meandir/*.tmp-* 2> /dev/null

    for c in $numclients ; do

      mangle_in="c=$c.$mangle_out"

      echo $mangle_in

      for x in $seed ; do

        tmp_out=tmp-$x

        # delay

        rm tmp 2> /dev/null
        for infile in out.$x.$mangle_in.* ; do
          cat $infile | $percentile --col 2 --mean --quantiles 0.90 0.95 0.99 >> tmp
        done

        res=$(grep '+-' tmp | percentile.py --col 1 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-mean.$tmp_out
        #res=$(grep ^'0.9 ' tmp | percentile.py --col 2 --quantile 0.90 | cut -f 2 -d ' ')
        res=$(grep ^'0.9 ' tmp | percentile.py --col 2 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-090.$tmp_out
        #res=$(grep ^'0.95 ' tmp | percentile.py --col 2 --quantile 0.95 | cut -f 2 -d ' ')
        res=$(grep ^'0.95 ' tmp | percentile.py --col 2 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-095.$tmp_out
        #res=$(grep ^'0.99 ' tmp | percentile.py --col 2 --quantile 0.99 | cut -f 2 -d ' ')
        res=$(grep ^'0.99 ' tmp | percentile.py --col 2 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/out-099.$tmp_out

        # utilization

        cat util.$x.$mangle_in.* | $percentile --col 3 --mean > tmp

        res=$(grep +- tmp | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/util-mean.$tmp_out

        rm -f util-tmp
        for infile in util.$x.$mangle_in.* ; do
          cat $infile | $percentile --col 3 --mean | grep +- | cut -f 1 -d ' ' >> util-tmp
        done
        util_min=$(sort -n util-tmp | head -n 1)
        util_max=$(sort -n util-tmp | tail -n 1)
        res=$(echo "scale=2;$util_max - $util_min" | bc)
        echo "$c $res" >> $oldpwd/$meandir/util-fair.$tmp_out

        # network traffc

        cat ovsstat.$x.$mangle_in | $percentile --col -1 --mean > tmp

        res=$(grep +- tmp | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/tpt-mean.$tmp_out

        # loss

        res=$(cat loss.$x.$mangle_in.* | percentile.py --col 1 --mean | cut -f 1 -d ' ')
        echo "$c $res" >> $oldpwd/$meandir/loss-mean.$tmp_out

        loss_min=$(cat loss.$x.$mangle_in.* | sort -n | head -n 1)
        loss_max=$(cat loss.$x.$mangle_in.* | sort -n | tail -n 1)
        res=$(echo "scale=2;$loss_max - $loss_min" | bc)
        echo "$c $res" >> $oldpwd/$meandir/loss-fair.$tmp_out

      done
    done

    for metric in $metrics ; do
      $confidence --xcol 1 --ycol 2 $oldpwd/$meandir/$metric.tmp-* > $oldpwd/$meandir/$metric.$mangle_out
    done

  done

  rm $oldpwd/$meandir/*.tmp-* 2> /dev/null

  cd $oldpwd
}

function histo {
  echo "unimplemented"
  return

  histodir=derived/histo

  mkdir -p $histodir 2> /dev/null

  oldpwd=$PWD
  cd results

  for c in $numclients ; do
  for e in $experiments ; do

    mangle="c=$c.e=$e.a=$a"
    echo $mangle

    outfile=$oldpwd/$histodir/out-q90-$mangle.dat
    rm -f xxx
    for infile in $(ls -1 out.*.$mangle.*) ; do
      $percentile --col 2 --quantile 0.9 < $infile | cut -f 2 -d ' ' >> xxx
    done
    sort -n xxx > $outfile

    outfile=$oldpwd/$histodir/tpt-$mangle.dat
    cat ovsstat.*.$mangle | $percentile --col -1 --mean > $outfile

    outfile=$oldpwd/$histodir/loss-$mangle.dat
    cat loss.*.$mangle.* | sort -n > $outfile

    outfile=$oldpwd/$histodir/out-dist-$mangle.dat
    cat out.*.$mangle.* | $percentile --col 2 --cdf > $outfile

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
Syntax: `basename $0` <run|mean|histo|enumerate>
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
seed="1 2 3 4 5 6 7 8 9 10"
duration=60
bw="1"
delay="10000us"
sizes="1000"
losses="0" #1,3,5 - 10 non più
percentile="percentile.py --warmup_col 1 --warmup 10 --duration $duration"
confidence="confidence.py --alpha 0.05"
numclients="1 5"
script_name="linear_quic.py"

experiments="grpc quic quic0rtt"
$1
