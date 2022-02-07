CC=g++
CFLAGS=-Wall -g
PTHREADS=-lpthread

all: build-server_tcp build-client_tcp build-server_udp build-client_udp

tcp: build-server_tcp build-client_tcp

udp: build-server_udp build-client_udp

build-server_tcp:
	$(CC) $(CFLAGS) -o chatserver_tcp server_tcp/chatserver.cpp $(PTHREADS)

build-server_udp:
	$(CC) $(CFLAGS) -o chatserver_udp server_udp/chatserver.cpp $(PTHREADS)

build-client_tcp:
	$(CC) $(CFLAGS) -o chatclient_tcp client_tcp/chatclient.cpp $(PTHREADS)

build-client_udp:
	$(CC) $(CFLAGS) -o chatclient_udp client_udp/chatclient.cpp $(PTHREADS)

clean:
	rm -rf chatserver_* chatclient_* Submission.zip

submit:
	zip Submission.zip client_*/* server_*/* makefile
