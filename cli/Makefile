CC = gcc
QJSC = ../quickjs/qjsc
CFLAGS = -shared -fPIC -I../quickjs
LDFLAGS = -L../quickjs -lquickjs -lcurl

all: mycli

http.so: http.c
	$(CC) $(CFLAGS) -o http.so http.c $(LDFLAGS)

mycli: cli.js http.so
	$(QJSC) -o mycli -m cli.js

clean:
	rm -f mycli http.so

.PHONY: all clean