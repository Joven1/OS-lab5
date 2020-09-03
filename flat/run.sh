#!/bin/bash
cd os 
make clean; make
cd ..
cd apps/fdisk/
make clean; make; make run
cd ..
cd ostests
make clean; make; make run
cd ..
cd file_test
make clean; make; make run
#cd apps/example/make_procs
#make clean; make; make run
