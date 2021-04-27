#!/bin/bash -x
#PJM -N "PTRANS" # jobname
#PJM -S		# output statistics
#PJM --spath "results/%n.%j.stat"
#PJM -o "results/%n.%j.out"
#PJM -e "results/%n.%j.err"
#PJM -L rscgrp=small
#PJM -L elapse=00:10:00
#PJM -L node=32
#------- Program execution -------#

MPIOPT="-of results/%n.%j.out -oferr results/%n.%j.err"

# hostname  external address    IB address (used for comm. to compute nodes)
# login2:   134.160.188.26 --> 10.6.90.12
# login3:   134.160.188.27 --> 10.6.90.13
# login4:   134.160.188.28 --> 10.6.90.14

echo "Start"
for i in 4 8 16 32
do
  echo $i
  mpiexec $MPIOPT -n $i ./f-pltranstest2 10.6.90.12 4
done
