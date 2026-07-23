QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_control_engine

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_control_engine.cpp
