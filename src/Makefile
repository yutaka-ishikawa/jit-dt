#
# For K and hibiki, -DUSE_LOCKF is required
#
#CCFLAGS = -DUSE_LOCKF
#
INSTTOP ?= /home/hp150019/data/u01148/jit-dt
REGION ?= saitama

INSTBIN = $(INSTTOP)/bin
INSTINCL = $(INSTTOP)/include
INSTLIB =  $(INSTTOP)/lib
INSTETC =  $(INSTTOP)/etc
INSTREGION = $(INSTTOP)/$(REGION)

.PHONY: all
SERVER_CMDS=watch_and_transfer jit-dt jit-dt-hibiki jit-dt-kobe-hibiki jit-dt-suita-hibiki jit-dt-kobe-ofp testinotify
SERVER_CMDS2=http_daemon http_put
CLIENT_CMDS=jit-kwatch jit-kobewatch kwatcher lwatcher transgen \
	transtest transtest2 ptranstest ptranstest2 \
	ltranstest ltranstest2 pltranstest pltranstest2 \
	ktestgen.pl ltestgen.pl \
	jit-watch.sh jit-yiwatch jit-yi2watch jit-lwatch
FUGAKU_CLIENT_CMDS=jit-kwatch kwatcher transgen f-pltranstest2 jit-fugaku-lwatch lwatcher
GEN_COMMANDS = jit-fugaku-lwatch
CLIENT_CONFS=conf conf-test conf-saitama
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
else ifeq (fn01, $(findstring fn01, $(HOSTNAME)))
	MPICC = mpifccpx
	KCC = fccpx
	KCCFLAGS = -O3 -g
	FJCFLAGS = -Nclang

endif
CC = gcc
INSTALL = install
OBJS =	misclib.o regexplib.o translib.o jitclient.o jitkclient.o jitcclient.o \
	inotifylib2.o watch_and_transfer.o testinotify.o \
	kwatcher.o  lwatcher.o transtest.o transtest2.o transgen.o
OBJS2 = http_put.o http_daemon.o
POBJS =  ptranstest.o ptranstest2.o pjitclient.o
LIBS = libjitdt-k.a libjitdt-pk.a libjitdt-l.a libjitdt-pl.a libf-jitdt-pl.a
KLIB = jitclient.o jitkclient.o inotifylib2.o misclib.o regexplib.o
K-KLIB = k-jitclient.o k-jitkclient.o k-inotifylib2.o k-misclib.o k-regexplib.o
PKLIB = pjitclient.o jitkclient.o inotifylib2.o misclib.o
K-PKLIB = pjitclient.o k-jitkclient.o k-inotifylib2.o k-misclib.o
LLIB = jitclient.o jitcclient.o inotifylib2.o misclib.o regexplib.o
PLLIB = pjitclient.o jitcclient.o inotifylib2.o misclib.o
F-PLLIB = f-pjitclient.o f-jitcclient.o f-misclib.o 
CCFLAGS += -O3 -Wall -g -c
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
fugaku_client: $(FUGAKU_CLIENT_CMDS)
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
libf-jitdt-pl.a: $(F-PLLIB)
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
	[ -d $(INSTREGION) ] || mkdir -p $(INSTREGION)
	[ -d $(INSTREGION)/data ] || mkdir -p $(INSTREGION)/data
	[ -d $(INSTREGION)/bell ] || mkdir -p $(INSTREGION)/bell
	[ -d $(INSTREGION)/log ] || mkdir -p $(INSTREGION)/log
install_lib: libs
	[ -d $(INSTINCL) ] || mkdir -p $(INSTINCL)
	[ -d $(INSTLIB) ] || mkdir -p $(INSTLIB)
	$(INSTALL) jitclient.h misclib.h $(INSTINCL)
	$(INSTALL) $(LIBS) $(INSTLIB)
install_fugaku_client: fugaku_client jit-fugaku-lwatch
	[ -d $(INSTBIN) ] || mkdir -p $(INSTBIN)
	$(INSTALL) $(FUGAKU_CLIENT_CMDS) $(INSTBIN) 
	[ -d $(INSTETC) ] || mkdir -p $(INSTETC)
	$(INSTALL) -m 0644 $(CLIENT_CONFS) $(INSTETC)
	[ -d $(INSTREGION) ] || mkdir -p $(INSTREGION)
	[ -d $(INSTREGION)/data ] || mkdir -p $(INSTREGION)/data
	[ -d $(INSTREGION)/bell ] || mkdir -p $(INSTREGION)/bell
	[ -d $(INSTREGION)/log ] || mkdir -p $(INSTREGION)/log

jit-fugaku-lwatch: jit-fugaku-lwatch.in
	sed -e 's%<TOP>%TOP=$(INSTTOP)%' jit-fugaku-lwatch.in | \
	sed -e 's%<REGION>%REGION=$(REGION)%' > jit-fugaku-lwatch
	chmod +x jit-fugaku-lwatch

.PHONY: depend
depend:  $(OBJS:.o=.c)
	-@ for i in $^; do cpp -MM $$i | sed "s/\ [_a-zA-Z0-9][_a-zA-Z0-9]*\.c//g" >> depend.inc; done
.PHONY: clean
clean:
	rm -f $(OBJS) $(LIBS) $(POBJS) $(GEN_COMMANDS) depend.inc
	rm -f *.o
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
testinotify: testinotify.o inotifylib2.o misclib.o
	$(CC) -o $@ $^
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
#
f-pltranstest2: ptranstest2.o libf-jitdt-pl.a
	$(MPICC) -o $@ $< -L. -lf-jitdt-pl
#	$(MPICC) -o $@ $< pjitclient.o
#
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
f-misclib.o: misclib.c
	$(KCC) $(KCCFLAGS) -o $@ -c $<
f-pjitclient.o: jitclient.c
	$(MPICC) $(MPICCFLAGS) -o $@ -c $<
f-jitcclient.o: jitcclient.c
	$(KCC) $(KCCFLAGS) -o $@ -c $<
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
