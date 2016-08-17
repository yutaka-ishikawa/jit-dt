.PHONY: all
CC = gcc
CCFLAGS = -O3 -Wall -g -c -I../libmicrohttpd-0.9.47/src/include -I../curl-7.46.0/include/
#LDFLAGS = -L../libmicrohttpd-0.9.47/src/microhttpd/.libs -lmicrohttpd
LDFLAGS_SERVER = ../libmicrohttpd-0.9.47/src/microhttpd/.libs/libmicrohttpd.a \
	-L$(HOME)/local/lib -lgcrypt -lgnutls -lpthread
LDFLAGS_CLIENT = ../curl-7.46.0/lib/.libs/libcurl.a -lssl -lcrypto -lz -lrt
.c.o:
	$(CC) $(CCFLAGS) -c $< -DDEBUG

all: http_daemon http_put watch_and_transfer watch2_and_transfer kwatcher transtest transgen locktest
http_daemon: http_daemon.o
	$(CC) -o http_daemon http_daemon.o $(LDFLAGS_SERVER) 
http_put: http_put.o
	$(CC) -o http_put http_put.o $(LDFLAGS_CLIENT) 
watch_and_transfer: watch_and_transfer.o translib.o translocklib.o inotifylib.o
	$(CC) -o watch_and_transfer watch_and_transfer.o translib.o translocklib.o inotifylib.o $(LDFLAGS_CLIENT)
watch2_and_transfer: watch2_and_transfer.o translib.o translocklib.o
	$(CC) -o watch2_and_transfer watch2_and_transfer.o translib.o translocklib.o $(LDFLAGS_CLIENT)
kwatcher: kwatcher.o translib.o translocklib.o inotifylib.o
	$(CC) -o kwatcher kwatcher.o translib.o translocklib.o inotifylib.o $(LDFLAGS_CLIENT)
transtest: transtest.o translocklib.o
	$(CC) -o transtest transtest.o translocklib.o
watch_and_transfer.o : translib.h
translib.o : translib.h translib.c
translocklib.o : translib.h translocklib.c
inotifylib.o : inotifylib.c
transtest.o : transtest.c
transgen.o : transgen.c
transgen: transgen.o translocklib.o
	$(CC) -o transgen transgen.o translocklib.o
locktest: locktest.c
	$(CC) -o locktest locktest.c
tar:
	tar czf 123.tar.gz Makefile *.h *.c perf.sh
clean:
	rm -f http_daemon watch_and_transfer http_put transtest locktest *.o
###
###
LDFLAGS_CLIENT2 = ../curl-7.46.0/lib/.libs/libcurl.a clk.o  -lssl -lcrypto -lz -lrt
http_daemon2: http_daemon.o clk.o
	$(CC) -o http_daemon http_daemon.o $(LDFLAGS_SERVER) clk.o
http_put2: http_put.o clk.o
	$(CC) -o http_put http_put.o $(LDFLAGS_CLIENT2)
watch_and_transfer2: watch_and_transfer.o translib.o translocklib.o clk.o
	$(CC) -o watch_and_transfer watch_and_transfer.o translib.o translocklib.o $(LDFLAGS_CLIENT2)
watch2_and_transfer2: watch2_and_transfer.o translib.o translocklib.o clk.o
	$(CC) -o watch2_and_transfer watch2_and_transfer.o translib.o translocklib.o $(LDFLAGS_CLIENT2)
