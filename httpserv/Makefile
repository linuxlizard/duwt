CC=gcc
CFLAGS=-g -Wall -Wpedantic -Wextra -Wshadow -Wcast-qual
LDFLAGS=-lmicrohttpd

PKGS=libnl-3.0 libnl-genl-3.0 jansson
CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LDFLAGS+=$(shell pkg-config --libs $(PKGS))

CFLAGS+=-I../libiw
LDFLAGS+=-L../libiw -liw

CPPFLAGS=$(CFLAGS) -std=gnu++17

all:sitesurvd

sitesurvd:sitesurvd.o bss_json.o
	$(CC) $(LDFLAGS) -o $@ $^ -L../libiw -liw -lstdc++

#sitesurvd.o:sitesurvd.c

sitesurvd.o:sitesurvd.cc

bss_json.o:../libiw/bss_json.c ../libiw/bss_json.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o sitesurvd
