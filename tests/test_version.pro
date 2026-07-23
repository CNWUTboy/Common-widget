QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_version

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_version.cpp
