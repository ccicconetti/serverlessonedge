#!/bin/bash

pids=""
numcomputers=5

echo "prepare configuration files"
for (( i = 0 ; i < numcomputers ; i++ )) ; do
  ./edgecomputer --json-example | sed -e "s/clambda@1/f$i/" > computer-$i.json
done

echo "start the e-controller"
env $LOG ./edgecontroller >& edgecontroller.log &
pids="$pids $!"
sleep 0.5

echo "start the e-computers"
for (( i = 0 ; i < numcomputers ; i++ )) ; do
  env $LOG ./edgecomputer \
    --conf type=file,path=computer-$i.json \
    --controller 127.0.0.1:6475 \
    --server-endpoint 127.0.0.1:$((10000+i)) \
    --state-endpoint 127.0.0.1:$((10100+i)) \
    --asynchronous \
    --companion-endpoint 127.0.0.1:6473 \
    >& edgecomputer-$i.log &
  pids="$pids $!"
done

echo "start the e-router"
env $LOG ./edgerouter \
  --server-endpoint 127.0.0.1:6473 \
    --controller 127.0.0.1:6475 \
    >& edgerouter.log &
pids="$pids $!"

echo "print e-router table"
while (true) ; do
  ret=$(./forwardingtableclient \
    --server-endpoint 127.0.0.1:6474 2> /dev/null)
  echo $ret | grep 'f0' >& /dev/null
  if [ $? -eq 0 ] ; then
    echo $ret
    break
  fi
  sleep 0.5
done

# wait for the user to continue, then clean up everything
read -n 1 -p "press any key to kill all the processes spawned"

for pid in $pids ; do
  kill $pid 2> /dev/null
done
rm -f computer-[0-3].json

# print possible processes still around
if [ "$(ps ax | grep edge | grep -v grep)" != "" ] ; then
  echo "there might be still processes alive"
fi
