CC=gcc
CFLAGS = -Wall
# uncomment this for SunOS
# LIBS = -lsocket -lnsl

all: sdohd

sdohd: 
	$(CC) -o sdohd sdohd.c $(LIBS)


clean:
	rm sdohd
