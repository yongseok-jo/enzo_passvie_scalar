#!/bin/sh  

# This makes enzo.exe on Happiness@SNU

echo "Making enzo on"
pwd
make clean
make default
cd ../../
./configure
cd src/enzo/
#make machine-tacc-ranger
module mpich-3.3.1-intel
#make machine-linux-openmpi
make machine-linux-mpich
#make precision-32 particles-64 integers-32 use-hdf4-yes packed-amr-yes taskmap-no fluxfix-no max-baryons-30 gravity-4s-no particle-id-32 opt-high
make precision-64 integers-32 particle-id-32 max-baryons-30 opt-high lcaperf-no make-tasks-per-node-36 
make show-config
make show-flags
make -j3
echo "Make done!"
pwd
date
