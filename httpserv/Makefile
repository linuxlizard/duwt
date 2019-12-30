CC=gcc
CFLAGS=-g -Wall -Wpedantic -Wextra 
LDFLAGS=-lmicrohttpd

PKGS=libnl-3.0 libnl-genl-3.0 
CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LDFLAGS+=$(shell pkg-config --libs $(PKGS))

CFLAGS+=-I../libiw
LDFLAGS+=-L../libiw -liw

all:sitesurvd

sitesurvd:sitesurvd.o
	$(CC) $(LDFLAGS) -o $@ $^ -lev -L../libiw -liw

sitesurvd.o:sitesurvd.c


clean:
	$(RM) *.o sitesurvd
