TEMPLATE = app
TARGET = Galileo

CONFIG += ordered debug c++17 warn_on
QT += sql widgets

# TODO how to enable these flags? this doesn't seem to work
CFLAGS+=-Wall -Wextra -Wshadow -pedantic
CPPFLAGS+=-Wall -Wextra -Wshadow -pedantic

HEADERS += galileo.h\
		 mainwindow.h\
		 bssmodel.h\
		 aboutdialog.h

SOURCES += galileo.cc\
		   ../src/iw.cc\
		   ../src/cfg80211.cc\
		   ../src/ie.cc\
		   ../src/attr.cc\
		   ../src/logging.cc\
		   mainwindow.cc\
		   bssmodel.cc\
		   aboutdialog.cc

INCLUDEPATH += ../include
INCLUDEPATH += ../fmt/include
LIBS += -L../fmt -lfmt

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

FORMS += aboutdialog.ui

requires(qtConfig(tableview))



# vim: ft=make
