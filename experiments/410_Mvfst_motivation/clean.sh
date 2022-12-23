#!/bin/bash

if [[ $# -ne 1 ]] ; then
  echo "syntax: `basename $0` <derived|log|cert|all>"
fi

if [[ $1 == "cert" || $1 == "all" ]] ; then
  rm -f ca.crt ca.key client.crt client.csr client.key server.crt server.csr server.key
elif [[ $1 == "derived" || $1 == "all" ]] ; then
  rm -rf derived/*
elif [[ $1 == "log" || $1 == "all" ]] ; then
  rm -f log.* log/*
elif [[ $1 == "all" ]] ; then
  rm -f results/*
  rm -f *.json
  rm -f *.dot *.png *.svg
  rm -f dist.txt
else
  echo "invalid command: $1"
  exit 1
fi

