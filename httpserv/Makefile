CC=gcc
CXX=g++
CFLAGS=-g -Wall -Wpedantic -Wextra -Wshadow -Wcast-qual
LDFLAGS=-g
LDLIBS=-lmicrohttpd

PKGS=libnl-3.0 libnl-genl-3.0 jansson
CFLAGS+=$(shell pkg-config --cflags $(PKGS))
LDLIBS+=$(shell pkg-config --libs $(PKGS))

CFLAGS+=-I../libiw
LDFLAGS+=-L../libiw 
LDLIBS+=-liw

LDLIBS+=-lstdc++ -lstdc++fs

CPPFLAGS=$(CFLAGS) -std=gnu++17

all:sitesurvd test_mimetypes

sitesurvd:sitesurvd.o bss_json.o mimetypes.o
#	$(CXX) -o $@ $^ $(LDFLAGS) -L../libiw -liw -lstdc++

#sitesurvd.o:sitesurvd.c

sitesurvd.o:sitesurvd.cc

bss_json.o:../libiw/bss_json.c ../libiw/bss_json.h
	$(CC) $(CFLAGS) -c -o $@ $<

test_mimetypes: test_mimetypes.o mimetypes.o
#	$(CXX) $(LDFLAGS) -o $@ $^

mimetypes.o:mimetypes.cc mimetypes.h

test_mimetypes.o:test_mimetypes.cc mimetypes.h

tst:tst.o
	$(CXX) -o $@ $^ -lstdc++ -lboost_locale

tst.o:tst.cc
	$(CC) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) *.o sitesurvd test_mimetypes
