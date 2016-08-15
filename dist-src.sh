#!/bin/bash
#
# git clone ishikawa@www.sys.aics.riken.jp:/var/aics-sys/git/jit-dt

SRC="README src/Makefile \
    src/http_daemon.c src/clk.c src/http_put.c \
    src/watch_and_transfer.c src/watch2_and_transfer.c \
    src/translib.h src/translib.c \
    src/translocklib.h src/translocklib.c src/transtest.c"

tar cf jit-dt-src.tar $SRC



