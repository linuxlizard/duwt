TEMPLATE = app
TARGET = scandump

CONFIG += console debug c++17

QT += sql 

HEADERS += ../iw.h\
		 ../netlink.hh\
		 ../util.h\
		 ../ie.hh\
		 ../attr.hh

SOURCES += ../iw.c\
		   ../netlink.cc\
		   ../util.c\
		   ../ie.cc\
		   ../attr.cc\
		   main.cc

INCLUDEPATH += ../
INCLUDEPATH += ../include
INCLUDEPATH += ../fmt/include

LIBS += -L../fmt -lfmt

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

# vim: ft=make
