# davep 20190402 copied from fmt/support/qmake.pro
# 	mild edits to allow fmt to build in my duwt environment
#
# Staticlib configuration for qmake builds
# For some reason qmake 3.1 fails to identify source dependencies and excludes format.cc and printf.cc
# from compilation so it _MUST_ be called as qmake -nodepend
# A workaround is implemented below: a custom compiler is defined which does not track dependencies

TEMPLATE = lib

TARGET = fmt

CONFIG += staticlib warn_on c++11

CONFIG -= qt

INCLUDEPATH += ./include

SOURCES = \
    ./src/format.cc \
    ./src/posix.cc

message("foo bar baz")

#fmt.name = libfmt
#fmt.input = FMT_SOURCES
#fmt.output = ${QMAKE_FILE_BASE}$$QMAKE_EXT_OBJ
#fmt.clean = ${QMAKE_FILE_BASE}$$QMAKE_EXT_OBJ
#fmt.depends = ${QMAKE_FILE_IN}
## QMAKE_RUN_CXX will not be expanded
#fmt.commands = $$QMAKE_CXX -I./include -c $$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO $$QMAKE_CXXFLAGS_CXX11 ${QMAKE_FILE_IN}
#fmt.variable_out = OBJECTS
#fmt.CONFIG = no_dependencies no_link
#fmt.includepath += ../include foobarbaz
#fmt.includepath += ../include/fmt
#fmt.INCLUDEPATH += ./include

#QMAKE_EXTRA_COMPILERS += fmt

# vim: ft=make
