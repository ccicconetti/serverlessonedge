#!/bin/bash

argc=$#
argv=($@)

if [ $argc -le 4 ] ; then
  echo "syntax: $0 W H X Y [files...]"
  exit 1
fi

tmpfile=/tmp/multiplot.sh.$$

width=$1
height=$2
cols=$3
rows=$4

plotw=$(( width  / cols ))
ploth=$(( height / rows ))

cur=4   # skip W, H, X, and Y
for (( i = 0 ; i < $cols ; i++ )) ; do
  for (( j = 0 ; j < $rows ; j++ )) ; do
    plotfile=${argv[cur]}

    x=$(( i * plotw ))
    y=$(( j * ploth ))

    if [ $cur -lt $argc ] ; then
      echo "set term x11 persist size $plotw,$ploth position $x,$y" > $tmpfile
      cat $plotfile >> $tmpfile
      gnuplot $tmpfile
      echo "$plotfile"
    fi
    cur=$(( cur + 1 ))
  done
done

rm -f $tmpfile

read -n 1 -s -p "press any key to close windows..."
killall gnuplot_x11

exit 0
