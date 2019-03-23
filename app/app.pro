TEMPLATE = app
TARGET = Galileo

CONFIG += ordered debug c++17
QT += sql widgets

# TODO how to enable these flags? this doesn't seem to work
CFLAGS+=-Wall -Wextra -Wshadow -pedantic
CPPFLAGS+=-Wall -Wextra -Wshadow -pedantic

HEADERS += galileo.h\
		 ../iw.h\
		 ../netlink.hh\
		 ../ie.hh\
		 ../util.h\
		 ../attr.hh\
		 mainwindow.h\
		 aboutdialog.h

SOURCES += galileo.cc\
		   ../iw.c\
		   ../netlink.cc\
		   ../ie.cc\
		   ../util.c\
		   ../attr.cc\
		   mainwindow.cc\
		   aboutdialog.cc

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

FORMS += mainwindow.ui aboutdialog.ui

requires(qtConfig(tableview))



# vim: ft=make
