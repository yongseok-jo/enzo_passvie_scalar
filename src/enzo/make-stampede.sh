#!/bin/sh  

# This makes enzo.exe on STAMPEDE@TACC

cd /home1/01903/tg811987/enzo-google-hg/enzo-unstable-GMC2/src/enzo/
echo "Making enzo on"
pwd
make clean
make default
cd /home1/01903/tg811987/enzo-google-hg/enzo-unstable-GMC2/
./configure
cd /home1/01903/tg811987/enzo-google-hg/enzo-unstable-GMC2/src/enzo/
#make machine-tacc-ranger
make machine-tacc-stampede
#make precision-32 particles-64 integers-32 use-hdf4-yes packed-amr-yes taskmap-no fluxfix-no max-baryons-30 gravity-4s-no particle-id-32 opt-high
make precision-32 integers-32 particle-id-32 max-baryons-30 opt-high lcaperf-no
make show-config
make show-flags
make -j3
echo "Make done!"
pwd
date
