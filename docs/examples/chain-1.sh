#!/bin/bash

pids=""

echo "prepare configuration files"
./edgecomputer --json-example | sed -e "s/clambda@1/f1/" > computer-1.json
./edgecomputer --json-example | sed -e "s/clambda@1/f2/" > computer-2.json

echo "start the e-controller"
./edgecontroller >& edgecontroller.log &
pids="$pids $!"
sleep 0.5

echo "start the e-computers"
for (( i = 1 ; i <= 2 ; i++ )) ; do
  ./edgecomputer \
    --conf type=file,path=computer-$i.json \
    --controller 127.0.0.1:6475 \
    --server-endpoint 127.0.0.1:$((10000+i)) \
    >& edgecomputer-$i.log &
  pids="$pids $!"
done

echo "start the e-router"
./edgerouter \
  --server-endpoint 127.0.0.1:6473 \
    --controller 127.0.0.1:6475 \
    >& edgerouter.log &
pids="$pids $!"

echo "print e-router table"
while (true) ; do
  ret=$(./forwardingtableclient \
    --server-endpoint 127.0.0.1:6474 2> /dev/null)
  echo $ret | grep 'f1' >& /dev/null
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
rm -f computer-[12].json
