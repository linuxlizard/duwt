TEMPLATE = app
TARGET = galileo

CONFIG += ordered debug c++17
QT += sql widgets

HEADERS += galileo.h\
		 ../iw.h\
		 ../netlink.hh\
		 ../ie.hh\
		 ../util.h\
		 mainwindow.h\
		 aboutdialog.h

SOURCES += galileo.cc\
		   ../iw.c\
		   ../netlink.cc\
		   ../ie.cc\
		   ../util.c\
		   mainwindow.cc\
		   aboutdialog.cc

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

FORMS += mainwindow.ui aboutdialog.ui

requires(qtConfig(tableview))



# vim: ft=make
