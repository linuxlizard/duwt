# davep 20191123 ; tinkering with libevent and libev
# http://www.wangafu.net/~nickm/libevent-book/TOC.html
# http://software.schmorp.de/pkg/libev.html
#
# uses ICU 
# 	http://site.icu-project.org/
# 	http://userguide.icu-project.org/
# 	https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/index.html

CC=gcc
PKGS=libnl-3.0 libnl-genl-3.0 icu-i18n
#PKGS=libnl-3.0 libnl-genl-3.0 libevent_core libevent libevent_extra
CFLAGS:=-g -Wall -Wpedantic -Wextra $(shell pkg-config --cflags $(PKGS))
# shamelessly copied extra warnings from m*lib https://github.com/P-p-H-d/mlib
CFLAGS+=-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith -Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code -Wlogical-op -Wstrict-aliasing=2 -Wredundant-decls -Wold-style-definition -Wno-unused-function

# http://userguide.icu-project.org/strings/utf-8
CFLAGS+=-DU_CHARSET_IS_UTF8=1

LDFLAGS:=-g $(shell pkg-config --libs $(PKGS))

CORE_H:=xassert.h log.h core.h hdump.h
CORE_O:=xassert.o log.o hdump.o util.o

all:scan-event-ev scan-dump

scan-event: scan-event.o hdump.o iw-scan.o nlnames.o log.o list_debug.o bug.o
	$(CC) $(LDFLAGS) -o $@ $^

scan-event.o: scan-event.c
	$(CC) $(CFLAGS) -c scan-event.c -o scan-event.o 

scan-event-ev: scan-event-ev.o $(CORE_O) iw-scan.o nlnames.o bytebuf.o ie.o list_debug.o bug.o
	$(CC) $(LDFLAGS) -lev -o $@ $^

scan-event-ev.o: scan-event-ev.c iw-scan.h bytebuf.h $(CORE_H)
	$(CC) $(CFLAGS) -c $< -o $@

scan-dump: scan-dump.o iw.o $(CORE_O) nlnames.o ie.o bss.o list_debug.o bug.o
	$(CC) $(LDFLAGS) -licuio -o $@ $^

scan-dump.o: scan-dump.c iw.h ie.h $(CORE_H)
	$(CC) $(CFLAGS) -c $< -o $@


hdump.o:hdump.c hdump.h $(CORE_H)
log.o:log.c log.h $(CORE_H)
bytebuf.o:bytebuf.c bytebuf.h $(CORE_H)
ie.o:ie.c ie.h $(CORE_H)
iw.o:iw.c iw.h $(CORE_H)
bss.o:bss.c bss.h $(CORE_H)
xassert.o:xassert.c xassert.h $(CORE_H)
iw-scan.o:iw-scan.c iw-scan.h bytebuf.h $(CORE_H)
list_debug.o:list_debug.c list.h bug.h $(CORE_H)
bug.o:bug.c bug.h $(CORE_H)
util.o:util.c $(CORE_H)

# nl80211.h uses duplicates for certain enum values so need to turn off the
# strict switch+enum checking
nlnames.o:nlnames.c nlnames.h
	$(CC) $(filter-out -Wswitch-enum,$(CFLAGS)) -c $< -o $@

test_xassert:test_xassert.o xassert.o
test_xassert.o:test_xassert.c xassert.h

test_ie:test_ie.o ie.o log.o xassert.o
test_ie.o:test_ie.c ie.h $(CORE_H)

test_bytebuf:test_bytebuf.o bytebuf.o xassert.o hdump.o log.o
test_bytebuf.o:test_bytebuf.c bytebuf.h xassert.h log.h

test_bss:test_bss.o bss.o ie.o iw.o list_debug.o nlnames.o bug.o $(CORE_O)
test_bss.o:test_bss.c $(CORE_H)

VALGRIND_OPTS=--error-exitcode=1 --leak-check=yes
test: test_bytebuf test_ie test_bss
	valgrind  $(VALGRIND_OPTS) ./test_bytebuf
	valgrind $(VALGRIND_OPTS)  ./test_ie
	valgrind $(VALGRIND_OPTS)  ./test_bss

clean:
	$(RM) *.o scan-event scan-event-ev test_bss test_bytebuf test_ie test_xassert scan-dump
