#!/bin/bash

mydir="xcagas01"

pushd . > /dev/null
mkdir ${mydir} && cd ${mydir}

dd if=/dev/random bs=1 count=$1 of=numbers > /dev/null 2>&1

mpic++ --prefix /usr/local/share/OpenMPI -o ots ../ots.cpp

mpirun --prefix /usr/local/share/OpenMPI -np $1 ots

popd > /dev/null
rm -rf ${mydir}

