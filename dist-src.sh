#!/bin/bash
#
# git clone ishikawa@www.sys.r-ccs.riken.jp:/var/aics-sys/git/jit-dt
# scp jit-dt-src.tar.gz www.sys.r-ccs.riken.jp:/data/www/html/released_software/software/jit-dt/
#

SRC="00README.txt src/Makefile \
  doc/Makefile		\
  doc/Makefile.sample	\
  doc/jit-dt.tex	\
  doc/*.png		\
  doc/jit-dt.pdf	\
  doc/*.c		\
  src/httplib.h		\
  src/jitcclient.h	\
  src/misclib.h		\
  src/translib.h	\
  src/inotifylib.h	\
  src/jitclient.h	\
  src/regexplib.h	\
  src/transtime.h	\
  src/http_daemon.c	\
  src/http_put.c	\
  src/inotifylib2.c	\
  src/jitcclient.c	\
  src/jitclient.c	\
  src/jitkclient.c	\
  src/kwatcher.c	\
  src/lwatcher.c	\
  src/misclib.c	\
  src/regexplib.c	\
  src/transgen.c	\
  src/translib.c	\
  src/transtest.c	\
  src/transtest2.c	\
  src/watch_and_transfer.c	\
  src/ktestgen.pl	\
  src/ltestgen.pl"

tar czf jit-dt-src.tar.gz $SRC
