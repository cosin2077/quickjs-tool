CC = gcc
CFLAGS = -Iquickjs -Wall
LDFLAGS = -Lquickjs -lquickjs -lm -ldl -lcurl

all: mytool

mytool: main.c
	$(CC) $(CFLAGS) main.c -o mytool $(LDFLAGS)

clean:
	rm -f mytool