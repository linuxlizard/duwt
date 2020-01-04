# davep 20191123 ; tinkering with libevent and libev
# http://www.wangafu.net/~nickm/libevent-book/TOC.html
# http://software.schmorp.de/pkg/libev.html
#
# uses ICU 
# 	http://site.icu-project.org/
# 	http://userguide.icu-project.org/
# 	https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/index.html

CC=gcc
PKGS=libnl-3.0 libnl-genl-3.0 jansson
#PKGS=libnl-3.0 libnl-genl-3.0 libevent_core libevent libevent_extra
CFLAGS:=-g -Wall -Wpedantic -Wextra $(shell pkg-config --cflags $(PKGS))
# shamelessly copied extra warnings from m*lib https://github.com/P-p-H-d/mlib
CFLAGS+=-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wswitch-default -Wswitch-enum -Wcast-align -Wbad-function-cast -Wstrict-overflow=5 -Winline -Wundef -Wnested-externs -Wshadow -Wunreachable-code -Wlogical-op -Wstrict-aliasing=2 -Wredundant-decls -Wold-style-definition -Wno-unused-function

LDFLAGS:=-g $(shell pkg-config --libs $(PKGS))

# http://userguide.icu-project.org/strings/utf-8
ICU_CFLAGS += -DU_CHARSET_IS_UTF8=1 $(shell pkg-config --cflags icu-i18n)
ICU_LDFLAGS += $(shell pkg-config --libs icu-i18n) -licuio


CORE_H:=xassert.h log.h core.h hdump.h
CORE_O:=xassert.o log.o hdump.o util.o

TEST:=test_ie test_bss test_bytebuf test_ie_he

all:scan-event-ev scan-dump $(TEST) libiw.a

libiw.a: iw.o iw-scan.o bytebuf.o $(CORE_O) nlnames.o ie.o ie_he.o bss.o bug.o ie_print.o
	$(AR) rcs $@ $^

scan-event: scan-event.o hdump.o iw-scan.o nlnames.o log.o bug.o
	$(CC) $(LDFLAGS) -o $@ $^

scan-event.o: scan-event.c
	$(CC) $(CFLAGS) -c scan-event.c -o scan-event.o 

scan-event-ev: scan-event-ev.o $(CORE_O) iw-scan.o nlnames.o bytebuf.o ie.o ie_he.o bug.o
	$(CC) $(LDFLAGS) -lev -o $@ $^

scan-event-ev.o: scan-event-ev.c iw-scan.h bytebuf.h $(CORE_H)
	$(CC) $(CFLAGS) -c $< -o $@

scan-dump: scan-dump.o iw.o $(CORE_O) nlnames.o ie.o ie_he.o bss.o bug.o ie_print.o bss_json.o
	$(CC) $(LDFLAGS) $(ICU_LDFLAGS) -o $@ $^

scan-dump-lib: scan-dump.o 
	$(CC) $(LDFLAGS) $(ICU_LDFLAGS) -o $@ $^ -L../libiw -liw 

scan-dump.o: scan-dump.c iw.h ie.h ie_he.h ie_print.h $(CORE_H)
	$(CC) $(CFLAGS) $(ICU_CFLAGS) -c $< -o $@

.PHONY:tst
tst:
	gcc -g -Wall -o hello.o -c hello.c
	gcc -g -Wall -o tst.o -c tst.c
	ar rcs libhello.a hello.o xassert.o log.o
	ranlib libhello.a
	gcc -g -Wall -o tst tst.o -L. -lhello

hdump.o:hdump.c hdump.h $(CORE_H)
log.o:log.c log.h $(CORE_H)
bytebuf.o:bytebuf.c bytebuf.h $(CORE_H)
ie.o:ie.c ie.h ie_he.h $(CORE_H)
ie_he.o:ie_he.c ie_he.h $(CORE_H)
iw.o:iw.c iw.h $(CORE_H)
bss.o:bss.c bss.h $(CORE_H)
xassert.o:xassert.c xassert.h $(CORE_H)
iw-scan.o:iw-scan.c iw-scan.h bytebuf.h $(CORE_H)
bug.o:bug.c bug.h $(CORE_H)
util.o:util.c $(CORE_H)
bss_json.o:bss_json.c bss_json.h $(CORE_H)

# nl80211.h uses duplicates for certain enum values so need to turn off the
# strict switch+enum checking
nlnames.o:nlnames.c nlnames.h
	$(CC) $(filter-out -Wswitch-enum,$(CFLAGS)) -c $< -o $@

test_xassert:test_xassert.o xassert.o
test_xassert.o:test_xassert.c xassert.h

test_ie:test_ie.o ie.o ie_he.o ie_print.o $(CORE_O)
test_ie.o:test_ie.c ie.h ie_he.h ie_print.h $(CORE_H)

test_ie_he:test_ie_he.o ie.o ie_he.o ie_print.o $(CORE_O)
test_ie_he.o:test_ie_he.c ie.h ie_he.h ie_print.h $(CORE_H)

test_bytebuf:test_bytebuf.o bytebuf.o xassert.o hdump.o log.o
test_bytebuf.o:test_bytebuf.c bytebuf.h xassert.h log.h

test_bss:test_bss.o bss.o ie.o ie_he.o iw.o nlnames.o bug.o $(CORE_O)
test_bss.o:test_bss.c $(CORE_H)

VALGRIND_OPTS=--error-exitcode=1 --leak-check=yes --exit-on-first-error=yes
test: test_bytebuf test_ie test_bss test_ie_he
	valgrind  $(VALGRIND_OPTS) ./test_bytebuf
	valgrind $(VALGRIND_OPTS)  ./test_ie
	valgrind $(VALGRIND_OPTS)  ./test_ie_he
	valgrind $(VALGRIND_OPTS)  ./test_bss

clean:
	$(RM) *.o *.a scan-event scan-event-ev test_bss test_bytebuf test_ie test_xassert scan-dump
