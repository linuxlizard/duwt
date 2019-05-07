TEMPLATE = app
TARGET = scanevent

CONFIG += cmdline debug c++17 warn_on
CONFIG -= qt

# https://stackoverflow.com/questions/53022608/application-crashes-with-symbol-zdlpvm-version-qt-5-not-defined-in-file-libqt
QMAKE_CXXFLAGS += "-fno-sized-deallocation"

#QT += sql 

xxHEADERS += ../iw.h\
		 ../netlink.hh\
		 ../util.h\
		 ../ie.hh\
		 ../logging.h\
		 ../attr.hh

SOURCES += ../src/iw.cc\
		   ../src/cfg80211.cc\
		   ../src/ie.cc\
		   ../src/attr.cc\
		   ../src/logging.cc\
		   main.cc

INCLUDEPATH += ../include
INCLUDEPATH += ../fmt/include

LIBS += -L../fmt -lfmt

CONFIG += link_pkgconfig
PKGCONFIG += libnl-3.0 libnl-genl-3.0 libnl-route-3.0

# vim: ft=make
