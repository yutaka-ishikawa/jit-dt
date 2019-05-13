#!/bin/bash -x
#PJM -g gg10
#PJM -L rscgrp=debug-flat
#PJM -L elapse=00:10:00
#PJM -L node=32
echo "Start"

for i in 4 8 16 32
do
  echo $i
  mpiexec -n $i ./pltranstest2 172.30.130.102 4
done
