.PHONY: all
COMMANDS=http_daemon http_put watch_and_transfer kwatcher transtest ptranstest transgen
HOSTNAME=$(shell hostname)
MPICC = mpicc
FJCFLAGS =
ifeq (fe01, $(findstring fe01, $(HOSTNAME)))
	MPICC = mpifccpx
	FJCFLAGS = -Xg
endif
CC = gcc
INSTALL = install
INSTDIR = $(HOME)/bin
OBJS =	translib.o translocklib.o inotifylib.o watch_and_transfer.o \
	transtest.o transgen.o kwatcher.o \
	http_put.o http_daemon.o ptranstest.o ptranslocklib.o 
CCFLAGS = -DUSE_LOCKF -O3 -Wall -g -c \
	-I../libmicrohttpd-0.9.47/src/include -I../curl-7.46.0/include/
MPICCFLAGS = -DMPIENV $(CCFLAGS) $(FJCFLAGS)
LDFLAGS_SERVER = ../libmicrohttpd-0.9.47/src/microhttpd/.libs/libmicrohttpd.a \
	-L$(HOME)/local/lib -lgcrypt -lgnutls -lpthread -lrt
LDFLAGS_CLIENT = -L/usr/lib64 ../curl-7.46.0/lib/.libs/libcurl.a -lssl -lcrypto -lz -lrt
.c.o:
	$(CC) $(CCFLAGS) -c $< -DDEBUG

all: $(COMMANDS)

.PHONY: depend
depend:  $(OBJS:.o=.c)
	-@ for i in $^; do cpp -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.c//g" >> depend.inc; done
install:
	$(INSTALL) watch_and_transfer jit-dt $(INSTDIR) 
	$(INSTALL) kwatcher $(INSTDIR) 
	$(INSTALL) transtest ptranstest $(INSTDIR) 
.PHONY: clean
clean:
	rm -f $(OBJS) $(COMMANDS)

http_daemon: http_daemon.o
	$(CC) -o http_daemon $^ $(LDFLAGS_SERVER) 
http_put: http_put.o
	$(CC) -o http_put http_put.o $(LDFLAGS_CLIENT) 
watch_and_transfer: watch_and_transfer.o translib.o translocklib.o inotifylib.o
	$(CC) -o watch_and_transfer $^ $(LDFLAGS_CLIENT)
kwatcher: kwatcher.o translib.o translocklib.o inotifylib.o
	$(CC) -o kwatcher $^ $(LDFLAGS_CLIENT)
transtest: transtest.o translocklib.o
	$(CC) -o transtest $^
ptranstest: ptranstest.o ptranslocklib.o
	$(MPICC) -o ptranstest $^
ptranstest.o: transtest.c
	$(MPICC) $(MPICCFLAGS) -o ptranstest.o -c transtest.c
ptranslocklib.o: translocklib.c
	$(MPICC) $(MPICCFLAGS) -o ptranslocklib.o -c translocklib.c
transgen: transgen.o translocklib.o
	$(CC) -o transgen $^

-include depend.inc
