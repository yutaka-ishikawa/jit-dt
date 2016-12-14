#! /bin/bash -x
#PJM --rsc-list rscgrp=micro
#PJM --rsc-list node=4
#PJM --rsc-list elapse=0:10:00  # HH:MM:SS
###	#PJM --mpi use-rankdir

. /work/system/Env_base
pwd
echo "Start"
mpiexec -n 4 ./ptranstest /scratch/ra000022/a03228/data/
