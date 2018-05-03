#
# For K, -DUSE_LOCKF is required
#
CCFLAGS = -DUSE_LOCKF
#INSTTOP = /opt/nowcast
#INSTTOP = $(HOME)/nowcast
INSTTOP = /opt/nowcast/yitest2

.PHONY: all
SERVER_CMDS=watch_and_transfer jit-dt jit-dt-hibiki jit-dt-kobe-hibiki jit-dt-suita-hibiki
SERVER_CMDS2=http_daemon http_put
CLIENT_CMDS=jit-kwatch jit-kobewatch kwatcher lwatcher transgen \
	transtest transtest2 ptranstest ptranstest2 \
	ltranstest ltranstest2 pltranstest pltranstest2 \
	ktestgen.pl ltestgen.pl \
	jit-watch.sh jit-yiwatch jit-yi2watch
K_CLIENT_CMDS=jit-kwatch kwatcher transgen k-transtest2 k-ptranstest2
CLIENT_CONFS=conf conf-test
SERVER_CONFS=serverconf

HOSTNAME=$(shell hostname)
MPICC = mpicc
## for test pupose */
KCC = cc
FJCFLAGS =
ifeq (fe01, $(findstring fe01, $(HOSTNAME)))
	MPICC = mpifccpx
	KCC = fccpx 
	FJCFLAGS = -Xg
endif
CC = gcc
INSTALL = install
INSTBIN = $(INSTTOP)/bin
INSTINCL = $(INSTTOP)/include
INSTLIB =  $(INSTTOP)/lib
INSTETC =  $(INSTTOP)/etc
OBJS =	misclib.o regexplib.o translib.o jitclient.o jitkclient.o jitcclient.o \
	inotifylib2.o watch_and_transfer.o \
	kwatcher.o  lwatcher.o transtest.o transtest2.o transgen.o
OBJS2 = http_put.o http_daemon.o
POBJS =  ptranstest.o ptranstest2.o pjitclient.o
LIBS = libjitdt-k.a libjitdt-pk.a libjitdt-l.a libjitdt-pl.a
KLIB = jitclient.o jitkclient.o inotifylib2.o misclib.o regexplib.o
K-KLIB = k-jitclient.o k-jitkclient.o k-inotifylib2.o k-misclib.o k-regexplib.o
PKLIB = pjitclient.o jitkclient.o inotifylib2.o misclib.o
K-PKLIB = pjitclient.o k-jitkclient.o k-inotifylib2.o k-misclib.o
LLIB = jitclient.o jitcclient.o inotifylib2.o misclib.o regexplib.o
PLLIB = pjitclient.o jitcclient.o inotifylib2.o misclib.o
CCFLAGS += -O3 -Wall -g -c $(FJCFLAGS)
MPICCFLAGS += -DMPIENV $(CCFLAGS) $(FJCFLAGS)
LDFLAGS_HTTP_DAEMON = \
	../libmicrohttpd-0.9.47/src/microhttpd/.libs/libmicrohttpd.a \
	-L$(HOME)/local/lib -lgcrypt -lgnutls -lpthread -lrt
ifdef HTTP
   CCFLAGS += -DHTTP
   CCFLAGS += -I../libmicrohttpd-0.9.47/src/include -I../curl-7.46.0/include/
   LDFLAGS_HTTP_PUT = -L/usr/lib64 ../curl-7.46.0/lib/.libs/libcurl.a \
	-lssl -lcrypto -lz -lrt
   LDFLAGS_CLIENT = -L/usr/lib64 ../curl-7.46.0/lib/.libs/libcurl.a -lz -lrt
else
   LDFLAGS_HTTP_PUT =
   LDFLAGS_CLIENT = 
endif
#
.c.o:
	$(CC) $(CCFLAGS) -c $< -DDEBUG
#
all: server client libs
server: $(SERVER_CMDS)
client: $(CLIENT_CMDS)
k_client: $(K_CLIENT_CMDS)
libs: $(LIBS)
#
libjitdt-k.a: $(KLIB)
	ar rs $@ $^
libjitdt-pk.a: $(PKLIB)
	ar rs $@ $^
libjitdt-l.a: $(LLIB)
	ar rs $@ $^
libjitdt-pl.a: $(PLLIB)
	ar rs $@ $^
libk-jitdt-k.a: $(K-KLIB)
	ar rs $@ $^
libk-jitdt-pk.a: $(K-PKLIB)
	ar rs $@ $^

install: install_server install_client install_lib
install_server: server
	[ -d $(INSTBIN) ] || mkdir -p $(INSTBIN)
	$(INSTALL) $(SERVER_CMDS) $(INSTBIN) 
	[ -d $(INSTETC) ] || mkdir -p $(INSTETC)
	$(INSTALL) $(SERVER_CONFS) $(INSTETC) 
install_client: client
	[ -d $(INSTBIN) ] || mkdir -p $(INSTBIN)
	$(INSTALL) $(CLIENT_CMDS) $(INSTBIN) 
	[ -d $(INSTETC) ] || mkdir -p $(INSTETC)
	$(INSTALL) -m 0644 $(CLIENT_CONFS) $(INSTETC) 
install_lib: libs
	[ -d $(INSTINCL) ] || mkdir -p $(INSTINCL)
	[ -d $(INSTLIB) ] || mkdir -p $(INSTLIB)
	$(INSTALL) jitclient.h misclib.h $(INSTINCL)
	$(INSTALL) $(LIBS) $(INSTLIB)
install_k_client: k_client
	[ -d $(INSTBIN) ] || mkdir -p $(INSTBIN)
	$(INSTALL) $(K_CLIENT_CMDS) $(INSTBIN) 
	[ -d $(INSTETC) ] || mkdir -p $(INSTETC)
	$(INSTALL) -m 0644 $(CLIENT_CONFS) $(INSTETC) 

.PHONY: depend
depend:  $(OBJS:.o=.c)
	-@ for i in $^; do cpp -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.c//g" >> depend.inc; done
.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBS) $(POBJS) $(COMMANDS) depend.inc
#
http_daemon: http_daemon.o
	$(CC) -o $@ $^ $(LDFLAGS_HTTP_DAEMON) 
http_put: http_put.o
	$(CC) -o $@ $^ $(LDFLAGS_HTTP_PUT) 
watch_and_transfer: watch_and_transfer.o misclib.o translib.o inotifylib2.o regexplib.o
	$(CC) -o $@ $^ $(LDFLAGS_HTTP_PUT)
kwatcher: kwatcher.o libjitdt-k.a
	$(CC) -o $@ $^ $(LDFLAGS_CLIENT)
lwatcher: lwatcher.o libjitdt-l.a
	$(CC) -o $@ $^ $(LDFLAGS_CLIENT) -lpthread
transtest: transtest.o libjitdt-k.a
	$(CC) -o $@ $< -L. -ljitdt-k
ptranstest: ptranstest.o libk-jitdt-pk.a
	$(MPICC) -o $@ $< -L. -lk-jitdt-pk
transtest2: transtest2.o libk-jitdt-k.a
	$(CC) -o $@ $< -L. -lk-jitdt-k
ptranstest2: ptranstest2.o libjitdt-pk.a
	$(MPICC) -o $@ $< -L. -ljitdt-pk
k-transtest2: k-transtest2.o libk-jitdt-k.a
	$(KCC) -o $@ $< -L. -lk-jitdt-k
k-ptranstest2: k-ptranstest2.o libk-jitdt-pk.a
	$(MPICC) -o $@ $< -L. -lk-jitdt-pk
ltranstest: transtest.o libjitdt-l.a
	$(CC) -o $@ $< -L. -ljitdt-l
pltranstest: ptranstest.o libjitdt-pl.a
	$(MPICC) -o $@ $< -L. -ljitdt-pl
ltranstest2: transtest2.o libjitdt-l.a
	$(CC) -o $@ $< -L. -ljitdt-l
pltranstest2: ptranstest2.o libjitdt-pl.a
	$(MPICC) -o $@ $< -L. -ljitdt-pl
transgen: transgen.o misclib.o jitkclient.o
	$(CC) -o $@ $^
#
pjitclient.o: jitclient.c
	$(MPICC) $(MPICCFLAGS) -o $@ -c $<
ptranstest.o: transtest.c
	$(MPICC) $(MPICCFLAGS) -o $@ -c $<
ptranstest2.o: transtest2.c
	$(MPICC) $(MPICCFLAGS) -o $@ -c $<
#
#
k-jitclient.o: jitclient.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-jitkclient.o: jitkclient.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-inotifylib2.o: inotifylib2.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-misclib.o: misclib.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-regexplib.o: regexplib.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-transtest2.o: transtest2.c
	$(KCC) $(CCFLAGS) -o $@ -c $<
k-ptranstest2.o: transtest2.c
	$(MPICC) $(MPICCFLAGS) -o $@ -c $<

#kwatcher: kwatcher.o misclib.o jitkclient.o inotifylib.o
#	$(CC) -o kwatcher $^ $(LDFLAGS_CLIENT)
#lwatcher: lwatcher.o jitcclient.o  misclib.o inotifylib.o
#	$(CC) -o lwatcher $^ $(LDFLAGS_CLIENT) -lpthread
#transtest: transtest.o misclib.o jitclient.o jitkclient.o
#	$(CC) -o transtest $^
#ptranstest: ptranstest.o misclib.o pjitclient.o jitkclient.o
#	$(MPICC) -o ptranstest $^
#transtest2: transtest2.o misclib.o jitclient.o jitkclient.o
#	$(CC) -o transtest2 $^
#ptranstest2: ptranstest2.o misclib.o jitclient.o jitkclient.o
#	$(MPICC) -o ptranstest2 $^
#ltranstest: transtest.o misclib.o jitclient.o jitcclient.o
#	$(CC) -o ltranstest $^
#pltranstest: ptranstest.o misclib.o pjitclient.o jitcclient.o
#	$(MPICC) -o pltranstest $^
#ltranstest2: transtest2.o misclib.o jitclient.o jitcclient.o
#	$(CC) -o ltranstest2 $^
#pltranstest2: ptranstest2.o misclib.o pjitclient.o jitcclient.o
#	$(MPICC) -o pltranstest2 $^
#	$(INSTALL) kwatcher $(INSTBIN) 
#	$(INSTALL) lwatcher $(INSTBIN) 
#	$(INSTALL) transtest ptranstest $(INSTBIN) 
#	$(INSTALL) transtest2 ptranstest2 $(INSTBIN) 
#	$(INSTALL) ltranstest pltranstest $(INSTBIN) 
#	$(INSTALL) ltranstest2 pltranstest2 $(INSTBIN) 
#	$(INSTALL) ktestgen.pl ltestgen.pl $(INSTBIN)

-include depend.inc
