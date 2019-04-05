TEMPLATE = app
TARGET = scanevent

CONFIG += console debug c++17

# https://stackoverflow.com/questions/53022608/application-crashes-with-symbol-zdlpvm-version-qt-5-not-defined-in-file-libqt
QMAKE_CXXFLAGS += "-fno-sized-deallocation"

#QT += sql 

HEADERS += ../iw.h\
		 ../netlink.hh\
		 ../util.h\
		 ../ie.hh\
		 ../logging.h\
		 ../attr.hh

SOURCES += ../iw.c\
		   ../netlink.cc\
		   ../util.c\
		   ../ie.cc\
		   ../attr.cc\
		   ../logging.cc\
		   main.cc

INCLUDEPATH += ../
INCLUDEPATH += ../include
INCLUDEPATH += ../fmt/include

LIBS += -L../fmt -lfmt

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

# vim: ft=make
