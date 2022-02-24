#!/bin/bash

function print_help {
  cat << EOF
Syntax: `basename $0` <data files>
Accepted options:
  -c: use color
  -s: font size  (default: 22)
  -P: page size  (default: 10,6)
  -p: convert to EPS then to PDF
  -x: convert to PNG
EOF
  exit 0
}

color='mono'
fontsize='22'
pointsize='1.5'
pagesize='10,6'
EPS=0
PNG=0
OPTIONERROR=0
OPTIND=1
while getopts "h?cs:pP:x" opt; do
  case "$opt" in
    h|\?)
      print_help
      ;;
    c)  color='color'
      ;;
    s)  fontsize=$OPTARG
      ;;
    p)  EPS=1
      ;;
    P)  pagesize=$OPTARG
      ;;
    x)  PNG=1
      ;;
  esac
done

shift $((OPTIND-1)) # Shift off the options and optional --.

if [ $# -lt 1 ] ; then
  print_help
fi

if [ $EPS -eq 1 ] ; then
  gnuplot_term="set term postscript enhanced $color size $pagesize font \",$fontsize\""
else
  gnuplot_term="set term pdfcairo enhanced $color size $pagesize font \",$fontsize\""
fi

tmpfile=/tmp/plot.sh.$$
for i in $@ ; do
  outeps="${i/.plt/}.eps"
  outpng="${i/.plt/}.png"
  outpdf="${i/.plt/}.pdf"

  if [ $EPS -eq 1 ] ; then
    outfile=$outeps
  else
    outfile=$outpdf
  fi
  cat << EOF > $tmpfile
$gnuplot_term
set output "$outfile"
load "$i"
set output
EOF
  echo -n "Processing $i."
  gnuplot $tmpfile
  if [ $EPS -eq 1 ] ; then
    convert -flatten -density 150 -rotate 90 $outeps $outpdf
  fi
  if [ $PNG -eq 1  ] ; then
    echo -n "."
    if [ -r "$outeps" ] ; then
      infile=$outeps
    else
      infile=$outpdf
    fi
    convert -flatten -density 150 $infile $outpng 2> /dev/null
  fi
  echo ".done"
done
rm -f $tmpfile 2> /dev/null
