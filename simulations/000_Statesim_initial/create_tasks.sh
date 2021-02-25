#!/bin/bash

if [ ! -r tasks/batch_task.csv  ] ; then
  if [ ! -r spar/bin/activate ] ; then
    echo "Cannot find spar virtual environment"
    exit 1
  fi
  source spar/bin/activate
  mkdir tasks 2> /dev/null
  spar --duration 1 tasks
  rm -f tasks/batch_instace.csv
  deactivate
fi
