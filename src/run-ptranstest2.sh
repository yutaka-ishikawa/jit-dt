#!/bin/bash -x
#PJM --rsc-list rscgrp=micro
#PJM --rsc-list node=32
#PJM --rsc-list elapse=0:30:00  # HH:MM:SS
#PJM --mpi use-rankdir
. /work/system/Env_base
pwd
echo "Start"
for i in 4 8 16 32
do
  echo $i
  mpiexec -n $i ./k-ptranstest2 /scratch/ra000022/a03228/data/ 10
done
