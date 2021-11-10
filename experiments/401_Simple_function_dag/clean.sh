#!/bin/bash

if [[ $# -ne 1 ]] ; then
  echo "syntax: `basename $0` <derived|log|all>"
fi

if [[ $1 == "derived" || $1 == "all" ]] ; then
  rm -rf derived/*
fi
if [[ $1 == "log" || $1 == "all" ]] ; then
  rm -f log.* log/*
fi
if [[ $1 == "all" ]] ; then
  rm -f results/*
  rm -f *.json
  rm -f *.dot *.png *.svg
  rm -f dist.txt
fi

