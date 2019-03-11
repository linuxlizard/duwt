TEMPLATE = app
TARGET = netlink

CONFIG += debug c++14

QT += sql 

HEADERS += ../iw.h\
		 ../netlink.hh\
		 ../util.h

SOURCES += ../iw.c\
		   ../netlink.cc\
		   ../util.c\
		   main.cc

INCLUDEPATH += ../

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

# vim: ft=make
