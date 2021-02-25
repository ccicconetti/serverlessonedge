#!/bin/bash

if [[ $# -ne 1 ]] ; then
  echo "syntax: `basename $0` <derived|log|all>"
fi

if [[ $1 == "derived" || $1 == "all" ]] ; then
  rm -f derived/mean/* derived/histo/* computer*.json conf.json
fi
if [[ $1 == "log" || $1 == "all" ]] ; then
  rm -f log.* log/*
fi
if [[ $1 == "all" ]] ; then
  rm -f results/*
fi
