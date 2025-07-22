#!/bin/tcsh

set root = `pwd`
cd $root
if ( -e bin ) rm -rf bin
mkdir bin
cd $root
cd $root/lib
make all
cd $root/prog
make clean
make all
make install
cd $root/util 
cp *csh $root/bin
# these perl files are probably obsolete
cp *pl $root/bin
cd $root
cd weather_srv
make clean
make all
make install

#

