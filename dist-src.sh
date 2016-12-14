#!/bin/bash
#
# git clone ishikawa@www.sys.aics.riken.jp:/var/aics-sys/git/jit-dt

SRC="README src/Makefile \
    src/http_daemon.c src/clk.c src/http_put.c \
    src/watch_and_transfer.c src/watch2_and_transfer.c \
    src/translib.h src/translib.c \
    src/ctranslib.h src/ctranslib.c \
    src/transtime.h src/transtime.c \
    src/translocklib.h src/translocklib.c \
    src/inotifylib.h src/inotifylib.c \
    src/translocklib.h src/translocklib.c src/kwatcher.c \
    src/lwatcher.c \
    src/transtest.c src/locktest.c src/transgen.c \
    src/calc_delay.pl src/jit-dt src/jit-dt-aics src/run-test.sh"


tar cf jit-dt-src.tar $SRC



