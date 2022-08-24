#!/bin/bash

if [ ! -d mvfst ] ; then
  echo "**********************************************************************"
  echo "BUILDING MVFST"
  git clone https://github.com/facebookincubator/mvfst.git > /dev/null
  pushd mvfst > /dev/null
  ./getdeps.sh >& build.log
  RET=$?
  popd > /dev/null

  if [ $RET -ne 0 ] ; then
    echo "build failed, see mvfst/build.log for details"
    exit $RET
  fi

  ls mvfst/_build/mvfst/lib/*.a >& /dev/null
  RET=$?
  if [ $RET -ne 0 ] ; then
    echo "build seems successful, but there are no libraries in mvfst/_build/mvfst/lib"
    exit $RET
  fi

  echo "CREATING MVFST SYMLINKS"
  pushd mvfst/_build > /dev/null
  rm -rf include lib 2> /dev/null
  mkdir include lib 2> /dev/null
  pushd include > /dev/null
  for i in $(find .. -maxdepth 2 -name include -type d | grep -v '^../include' ) ; do ln -s $i/* . ; done
  popd > /dev/null
  pushd lib > /dev/null
  for i in $(find .. -maxdepth 2 -name lib -type d | grep -v '^../lib' ) ; do 
    pushd $i > /dev/null
    tar --no-recursion -c -f tmp.tar *
    popd > /dev/null
    tar xf $OLDPWD/tmp.tar 2> /dev/null
    rm -f $OLDPWD/tmp.tar 2> /dev/null
  done
  popd > /dev/null
  popd > /dev/null
fi