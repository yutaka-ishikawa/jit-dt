.PHONY: all
SERVER_CMDS=http_daemon http_put watch_and_transfer
CLIENT_CMDS=kwatcher lwatcher transtest ptranstest transgen ltranstest ltranstest2
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
OBJS =	misclib.o translib.o jitkclient.o inotifylib.o watch_and_transfer.o\
	transtest.o transgen.o kwatcher.o jitcclient.o lwatcher.o \
	http_put.o http_daemon.o
POBJ =  ptranstest.o pjitkclient.o 
CCFLAGS = -DUSE_LOCKF -O3 -Wall -g -c \
	-I../libmicrohttpd-0.9.47/src/include -I../curl-7.46.0/include/
MPICCFLAGS = -DMPIENV $(CCFLAGS) $(FJCFLAGS)
LDFLAGS_HTTP_DAEMON = \
	../libmicrohttpd-0.9.47/src/microhttpd/.libs/libmicrohttpd.a \
	-L$(HOME)/local/lib -lgcrypt -lgnutls -lpthread -lrt
LDFLAGS_HTTP_PUT = -L/usr/lib64 ../curl-7.46.0/lib/.libs/libcurl.a \
	-lssl -lcrypto -lz -lrt
LDFLAGS_CLIENT = -L/usr/lib64 ../curl-7.46.0/lib/.libs/libcurl.a -lz -lrt
#
.c.o:
	$(CC) $(CCFLAGS) -c $< -DDEBUG
#
all: server client
server: $(SERVER_CMDS)
client: $(CLIENT_CMDS)
install: install_server install_client
install_server: server
	$(INSTALL) watch_and_transfer jit-dt $(INSTDIR) 
install_client: client
	$(INSTALL) kwatcher $(INSTDIR) 
	$(INSTALL) lwatcher $(INSTDIR) 
	$(INSTALL) transtest ptranstest $(INSTDIR) 

.PHONY: depend
depend:  $(OBJS:.o=.c)
	-@ for i in $^; do cpp -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.c//g" >> depend.inc; done
.PHONY: clean
clean:
	rm -f $(OBJS) $(POBJ) $(COMMANDS)
#
http_daemon: http_daemon.o
	$(CC) -o http_daemon $^ $(LDFLAGS_HTTP_DAEMON) 
http_put: http_put.o
	$(CC) -o http_put http_put.o $(LDFLAGS_HTTP_PUT) 
watch_and_transfer: watch_and_transfer.o misclib.o translib.o inotifylib.o
	$(CC) -o watch_and_transfer $^ $(LDFLAGS_HTTP_PUT)
kwatcher: kwatcher.o misclib.o jitkclient.o inotifylib.o
	$(CC) -o kwatcher $^ $(LDFLAGS_CLIENT)
lwatcher: lwatcher.o jitcclient.o  misclib.o inotifylib.o
	$(CC) -o lwatcher $^ $(LDFLAGS_CLIENT) -lpthread
transtest: transtest.o misclib.o jitkclient.o
	$(CC) -o transtest $^
ltranstest: transtest.o misclib.o jitcclient.o
	$(CC) -o ltranstest $^
ltranstest2: transtest2.o misclib.o jitcclient.o
	$(CC) -o ltranstest2 $^
ptranstest: ptranstest.o misclib.o pjitkclient.o
	$(MPICC) -o ptranstest $^
ptranstest.o: transtest.c
	$(MPICC) $(MPICCFLAGS) -o ptranstest.o -c transtest.c
pjitkclient.o: jitkclient.c
	$(MPICC) $(MPICCFLAGS) -o pjitkclient.o -c jitkclient.c
transgen: transgen.o misclib.o jitkclient.o
	$(CC) -o transgen $^

-include depend.inc
