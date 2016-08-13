CC = gcc
CCFLAGS = -O3 -Wall -c -I../libmicrohttpd-0.9.47/src/include -I../curl-7.46.0/include/
#LDFLAGS = -L../libmicrohttpd-0.9.47/src/microhttpd/.libs -lmicrohttpd
LDFLAGS_SERVER = ../libmicrohttpd-0.9.47/src/microhttpd/.libs/libmicrohttpd.a \
	-L~/local/lib -lgcrypt -lgnutls -lpthread
LDFLAGS_CLIENT1 = ../curl-7.46.0/lib/.libs/libcurl.a -lssl -lcrypto -lz
LDFLAGS_CLIENT2 = ../curl-7.46.0/lib/.libs/libcurl.a clk.o -lz
.c.o:
	$(CC) $(CCFLAGS) -c $< -DDEBUG

all: http_daemon http_put watch_and_transfer
http_daemon: http_daemon.o
	$(CC) -o http_daemon http_daemon.o $(LDFLAGS_SERVER) 
http_daemon2: http_daemon.o clk.o
	$(CC) -o http_daemon http_daemon.o $(LDFLAGS_SERVER) clk.o
http_put: http_put.o
	$(CC) -o http_put http_put.o $(LDFLAGS_CLIENT1) 
http_put2: http_put.o clk.o
	$(CC) -o http_put http_put.o $(LDFLAGS_CLIENT2)
watch_and_transfer: watch_and_transfer.o translib.o
	$(CC) -o watch_and_transfer watch_and_transfer.o translib.o $(LDFLAGS_CLIENT1)
watch_and_transfer2: watch_and_transfer.o translib.o clk.o
	$(CC) -o watch_and_transfer watch_and_transfer.o translib.o $(LDFLAGS_CLIENT2)
watch_and_transfer.o : translib.h
translib.o : translib.h
tar:
	tar czf 123.tar.gz Makefile *.h *.c perf.sh
clean:
	rm -f http_daemon watch_and_transfer http_put *.o
