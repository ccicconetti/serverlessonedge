#!/bin/bash

function print_help {
  cat << EOF
Syntax: `basename $0` <data files>
Accepted options:
  -c: print command
  -l: log scale
  -s: style
  -X: x-axis range
  -Y: y-axis range
  -t: title
  -x: x-axis label
  -y: y-axis label
  -f: refresh plot every specified period, in s
  -n: print only the last N lines
  -k: key style
  -w: x11 terminal options
EOF
  exit 0
}

printcmd=0
logscale=
style='u 1:2 w lp'
xrange='[*:*]'
yrange='[*:*]'
title=
xlabel=
ylabel=
refresh=
lastn=
key="off"
x11opts=""
OPTIONERROR=0
OPTIND=1
while getopts "h?cl:X:Y:s:t:x:y:f:n:k:w:" opt; do
  case "$opt" in
    h|\?)
      print_help
      ;;
    c)  printcmd=1
      ;;
    l)  logscale=$OPTARG
      ;;
    X)  xrange=$OPTARG
      ;;
    Y)  yrange=$OPTARG
      ;;
    s)  style=$OPTARG
      ;;
    t)  title=$OPTARG
      ;;
    x)  xlabel=$OPTARG
      ;;
    y)  ylabel=$OPTARG
      ;;
    f)  refresh=$OPTARG
      ;;
    n)  lastn=$OPTARG
      ;;
    k)  key=$OPTARG
      ;;
    w)  x11opts=$OPTARG
      ;;
  esac
done

shift $((OPTIND-1)) # Shift off the options and optional --.

if [ $# -lt 1 ] ; then
  print_help
fi

if [ $OPTIONERROR -gt 0 ]; then
  exit -1
fi

tmpfile=/tmp/plot.sh.$$
command=
first=1
for i in $@ ; do
  if [ $first -eq 1 ] ; then
    first=0
    command="plot "
  else
    command="$command, "
  fi
  if [ "$lastn" != "" ] ; then
    command="$command '< tail -n$lastn $i' $style"
  else
    command="$command '$i' $style"
  fi
done

if [ "$logscale" != "" ] ; then
  logscale="set logscale $logscale"
fi
if [ "$title" != "" ] ; then
  title="set title \"$title\""
fi
if [ "$xlabel" != "" ] ; then
  xlabel="set xlabel \"$xlabel\""
fi
if [ "$ylabel" != "" ] ; then
  ylabel="set ylabel \"$ylabel\""
fi

persist_flag="persist"
pause_cmd=
reread_cmd=
if [ "$refresh" != "" ] ; then
  persist_flag="nopersist"
  pause_cmd="pause $refresh"
  reread_cmd="reread"
fi

cat << EOF > $tmpfile
set term x11 $persist_flag $x11opts
set grid
set key $key
set xrange $xrange
set yrange $yrange
$title
$xlabel
$ylabel
$logscale
$command
$pause_cmd
$reread_cmd
EOF


if [ $printcmd -eq 1 ] ; then
  cat $tmpfile
else
  gnuplot $tmpfile
fi
rm -f $tmpfile 2> /dev/null
