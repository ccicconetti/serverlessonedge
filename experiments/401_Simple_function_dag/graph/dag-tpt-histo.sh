#1/bin/bash

states="4"
lengths="7"
inputsizes="10000 100000"
experiments="embedded hopbyhop remotestate"

rm dag-tpt-histo-*.dat 2> /dev/null
for s in $states ; do
  for l in $lengths ; do
    if [ $s -eq $l ] ; then
      continue
    fi
    for i in $inputsizes ; do
      for e in $experiments ; do
        ret=$(tail -n 1 ../derived/tpt-e=$e.s=$s.l=$l.i=$i-avg.dat | cut -f 2,3 -d ' ')
        if [[ $e == "embedded" ]] ; then
          label="PureFaaS"
        elif [[ $e == "hopbyhop" ]] ; then
          label="StateProp"
        else
          label="StateLocal"
        fi
        echo "$label $ret" >> dag-tpt-histo-$s-$l-$i.dat
      done
    done
  done
done
