#!/bin/bash

client=$(find ../../ -name testparallelcalls | head -n 1)

if [ "$client" == "" ] ; then
  echo "could not find client executable"
  exit 1
fi

$client \
  --gtest_also_run_disabled_tests \
  --gtest_filter=TestParallelCalls.DISABLED_test_single \
  "$@"
